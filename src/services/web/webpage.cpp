#include "webpage.h"

#include <Arduino.h>
#include <pgmspace.h>
#include <cstdio>
#include <algorithm>
#include <map>
#include <string>
#include <utility>

#include "core/globals.h"

namespace {

struct SetInput {
    String name;
    int reps = 0;
    int repDuration = 0;
    int pauseBetween = 0;
    int pauseAfter = 0;
    int percentIntensity = 0;
};

constexpr size_t kExerciseIdHexLength = StorageService::ExerciseId{}.size() * 2;

String jsonEscape(const std::string& value) {
    String escaped;
    escaped.reserve(value.length() + 8);
    for (char c : value) {
        switch (c) {
        case '"':
            escaped += "\\\"";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '\b':
            escaped += "\\b";
            break;
        case '\f':
            escaped += "\\f";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buffer[7];
                std::snprintf(buffer, sizeof(buffer), "\\u%04X", static_cast<unsigned char>(c));
                escaped += buffer;
            } else {
                escaped += c;
            }
            break;
        }
    }
    return escaped;
}

String exerciseRecordToJson(const StorageService::ExerciseRecord& record) {
    String json = "{";
    json += "\"id\":\"";
    json += StorageService::toHex(record.id);
    json += "\",\"name\":\"";
    json += jsonEscape(record.exercise.name);
    json += "\",\"setCount\":";
    json += String(static_cast<unsigned long>(record.exercise.sets.size()));
    json += ",\"sets\":[";

    for (size_t i = 0; i < record.exercise.sets.size(); ++i) {
        const Set& set = record.exercise.sets[i];
        if (i > 0) {
            json += ",";
        }

        const int repDuration = set.reps.empty() ? 0 : set.reps.front().timeRep;
        const int pauseBetween = set.reps.empty() ? 0 : set.reps.front().timeRest;

        json += "{\"name\":\"";
        json += jsonEscape(set.label);
        json += "\",\"reps\":";
        json += String(static_cast<unsigned long>(set.reps.size()));
        json += ",\"repDuration\":";
        json += String(repDuration);
        json += ",\"pauseBetween\":";
        json += String(pauseBetween);
        json += ",\"pauseAfter\":";
        json += String(set.timePauseAfter);
        json += ",\"percentIntensity\":";
        json += String(set.percentMaxIntensity);
        json += "}";
    }

    json += "]}";
    return json;
}

const char INDEX_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Interval Timer</title>
    <style>
        :root { --accent: #ffa000; --accent-dark: #ff8f00; --surface: #ffffff; font-family: 'Segoe UI', Arial, sans-serif; color: #1f1f1f; }
        body { margin: 0; padding: 32px; background: #f0f2f5; }
        .card { max-width: 900px; margin: 0 auto; background: var(--surface); border-radius: 16px; padding: 28px 32px; box-shadow: 0 18px 40px rgba(31,31,31,0.12); }
        h1 { margin: 0; text-align: center; font-size: 2rem; }
        .primary-btn { display: inline-flex; align-items: center; justify-content: center; gap: 12px; width: 100%; padding: 20px 18px; font-size: 1.35rem; font-weight: 600; background: linear-gradient(135deg, var(--accent), var(--accent-dark)); color: #fff; border: none; border-radius: 12px; cursor: pointer; transition: transform 0.12s ease, box-shadow 0.12s ease; margin-top: 24px; }
        .primary-btn:hover { transform: translateY(-1px); box-shadow: 0 10px 22px rgba(255,152,0,0.28); }
        .primary-btn:active { transform: translateY(1px); box-shadow: none; }
        #exerciseSection { display: none; margin-top: 28px; }
        .list-header { display: flex; align-items: center; justify-content: space-between; gap: 12px; margin: 28px 0 12px; }
        .list-header h2 { margin: 0; font-size: 1.4rem; }
        .status { font-size: 0.9rem; color: #6b6b6b; }
        .status.status-error { color: #d93025; }
        .exercise-list { display: flex; flex-direction: column; gap: 12px; margin-bottom: 12px; }
        .exercise-item { display: flex; align-items: center; justify-content: space-between; width: 100%; padding: 14px 16px; font-size: 1rem; background: #f7f9fb; border-radius: 10px; border: 1px solid #e0e3e8; cursor: pointer; transition: border-color 0.12s ease, transform 0.12s ease, box-shadow 0.12s ease; }
        .exercise-item:hover { border-color: var(--accent); transform: translateY(-1px); box-shadow: 0 6px 14px rgba(255,152,0,0.15); }
        .exercise-item__name { font-weight: 600; }
        .exercise-item__meta { color: #4a4a4a; font-size: 0.9rem; }
        .empty-state { padding: 28px 16px; text-align: center; color: #6b6b6b; border: 1px dashed #ccd0d5; border-radius: 10px; background: rgba(255,255,255,0.55); }
        .table-wrapper { overflow-x: auto; margin-top: 20px; }
        table { border-collapse: collapse; width: 100%; min-width: 640px; }
        th, td { border: 1px solid #d6d6d6; padding: 12px 16px; text-align: center; }
        th { background: #f7f9fb; font-weight: 600; }
        .row-label { text-align: left; font-weight: 600; width: 180px; background: #f0f0f0; }
        .editable-cell { background: #ffcc58; }
        .editable-cell input { width: 100%; max-width: 120px; display: block; margin: 0 auto; padding: 8px 10px; font-size: 0.95rem; border: none; border-radius: 8px; background: rgba(255,255,255,0.82); box-shadow: inset 0 1px 2px rgba(0,0,0,0.12); box-sizing: border-box; }
        .editable-cell input:focus { outline: 2px solid #ff9800; background: #fff; }
        .actions { display: flex; flex-wrap: wrap; gap: 12px; margin-top: 28px; justify-content: flex-end; }
        .actions button { min-width: 140px; padding: 12px 18px; font-size: 1rem; font-weight: 600; border-radius: 10px; border: none; cursor: pointer; }
        .secondary-btn { background: #e0e0e0; color: #333; padding: 12px 18px; font-size: 1rem; font-weight: 600; border-radius: 10px; border: none; cursor: pointer; }
        .secondary-btn:hover { background: #d6d6d6; }
        .danger-btn { background: #f44336; color: #fff; }
        .danger-btn:hover { background: #d32f2f; }
        .save-btn { background: var(--accent); color: #fff; }
        .save-btn:hover { background: var(--accent-dark); }
        .icon-btn { width: 48px; height: 48px; border-radius: 50%; border: none; background: var(--accent); color: #fff; font-size: 1.75rem; font-weight: 600; cursor: pointer; box-shadow: 0 6px 16px rgba(255,152,0,0.25); transition: transform 0.1s ease; }
        .icon-btn:hover { transform: translateY(-1px); }
        .icon-btn:active { transform: translateY(1px); box-shadow: none; }
        .layout-grid { display: flex; align-items: center; justify-content: space-between; gap: 18px; }
        .layout-grid h2 { margin: 0; font-size: 1.6rem; }
        .exercise-name-input { margin-top: 18px; display: flex; align-items: center; gap: 12px; }
        .exercise-name-input label { font-weight: 600; }
        .exercise-name-input input { flex: 1; padding: 12px 14px; border-radius: 8px; border: 1px solid #d6d6d6; font-size: 1rem; }
        @media (max-width: 720px) {
            body { padding: 20px; }
            .card { padding: 20px; }
            table { min-width: 520px; }
            .exercise-item { flex-direction: column; align-items: flex-start; gap: 4px; }
        }
    </style>
</head>
<body>
    <div class="card">
        <h1>Interval Timer</h1>
        <div class="list-header">
            <h2>Saved Exercises</h2>
            <span id="exerciseStatus" class="status"></span>
        </div>
        <div id="exerciseList" class="exercise-list">
            <div class="empty-state">Noch keine Übungen gespeichert.</div>
        </div>

        <button id="addExerciseBtn" class="primary-btn">+ Add Exercise</button>

        <form id="exerciseSection" class="exercise-form" action="/submit" method="GET">
            <input type="hidden" id="exerciseId" name="exerciseId">
            <div class="layout-grid">
                <h2 id="formTitle">Neue Übung</h2>
                <button type="button" id="closeFormBtn" class="secondary-btn">Zurück</button>
            </div>

            <div class="exercise-name-input">
                <label for="exerciseName">Exercise Name:</label>
                <input id="exerciseName" name="exerciseName" type="text" placeholder="Pushups" required maxlength="64">
            </div>

            <div class="table-wrapper">
                <table>
                    <tbody>
                        <tr id="exerciseNameRow">
                            <th class="row-label" scope="row">Exercise Name</th>
                            <td class="editable-cell" id="exerciseNameCell" colspan="1">
                                <input type="text" id="exerciseNameMirror" placeholder="Pushups" maxlength="64">
                            </td>
                            <td rowspan="7" id="addSetCell">
                                <button type="button" id="addSetBtn" class="icon-btn" title="Add set">+</button>
                            </td>
                        </tr>
                        <tr id="setLabelRow">
                            <th class="row-label" scope="row">Set No.</th>
                        </tr>
                        <tr id="repCountRow">
                            <th class="row-label" scope="row">Reps</th>
                        </tr>
                        <tr id="repDurationRow">
                            <th class="row-label" scope="row">Rep duration (s)</th>
                        </tr>
                        <tr id="pauseBetweenRow">
                            <th class="row-label" scope="row">Pause between reps (s)</th>
                        </tr>
                        <tr id="pauseAfterRow">
                            <th class="row-label" scope="row">Pause after set (s)</th>
                        </tr>
                        <tr id="percentIntensityRow">
                            <th class="row-label" scope="row">Percent Intensity (%)</th>
                        </tr>
                    </tbody>
                </table>
            </div>

            <div class="actions">
                <button type="button" id="deleteExerciseBtn" class="danger-btn" style="display:none;">Löschen</button>
                <button type="button" id="cancelBtn" class="secondary-btn">Zurück</button>
                <button type="submit" class="save-btn">Speichern</button>
            </div>
        </form>
    </div>

    <script>
    (() => {
        const state = { exercises: [] };

        const MAX_SETS = 15; // keep in sync with StorageService::kMaxSets
        const MAX_REPS_PER_SET = 20; // keep in sync with StorageService::kMaxRepsPerSet

        const addExerciseBtn = document.getElementById('addExerciseBtn');
        const exerciseSection = document.getElementById('exerciseSection');
        const exerciseListContainer = document.getElementById('exerciseList');
        const exerciseStatus = document.getElementById('exerciseStatus');
        const addSetBtn = document.getElementById('addSetBtn');
        const addSetCell = document.getElementById('addSetCell');
        const exerciseNameCell = document.getElementById('exerciseNameCell');
        const exerciseNameInput = document.getElementById('exerciseName');
        const exerciseNameMirror = document.getElementById('exerciseNameMirror');
        const exerciseIdInput = document.getElementById('exerciseId');
        const formTitle = document.getElementById('formTitle');
        const cancelButtons = [document.getElementById('cancelBtn'), document.getElementById('closeFormBtn')];
        const deleteExerciseBtn = document.getElementById('deleteExerciseBtn');

        const rowDefinitions = [
            { rowId: 'setLabelRow', name: 'name', type: 'text', placeholder: 'Set name', maxLength: '64', required: true, defaultValue: (i) => `Set ${i + 1}` },
            { rowId: 'repCountRow', name: 'reps', type: 'number', placeholder: '3', min: '1', max: String(MAX_REPS_PER_SET), step: '1', required: true, defaultValue: () => '3' },
            { rowId: 'repDurationRow', name: 'repDuration', type: 'number', placeholder: '7', min: '1', step: '1', required: true, defaultValue: () => '7' },
            { rowId: 'pauseBetweenRow', name: 'pauseBetween', type: 'number', placeholder: '30', min: '0', step: '1', required: true, defaultValue: () => '30' },
            { rowId: 'pauseAfterRow', name: 'pauseAfter', type: 'number', placeholder: '180', min: '0', step: '1', required: true, defaultValue: () => '180' },
            { rowId: 'percentIntensityRow', name: 'percentIntensity', type: 'number', placeholder: '50', min: '0', step: '1', required: true, defaultValue: () => '50' }
        ];

        let setCount = 0;
        let formMode = 'create';

        const escapeHtml = (value) => {
            const source = value === undefined || value === null ? '' : value;
            return String(source).replace(/[&<>"']/g, (ch) => {
                const map = { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' };
                return map[ch] || ch;
            });
        };

        const escapeAttr = (value) => {
            const source = value === undefined || value === null ? '' : value;
            return String(source).replace(/["&<>]/g, (ch) => {
                const map = { '"': '&quot;', '&': '&amp;', '<': '&lt;', '>': '&gt;' };
                return map[ch] || ch;
            });
        };

        const setStatus = (text, isError = false) => {
            if (!exerciseStatus) {
                return;
            }
            exerciseStatus.textContent = text || '';
            exerciseStatus.classList.toggle('status-error', Boolean(isError));
        };

        addSetCell.rowSpan = rowDefinitions.length + 1;

        const syncValue = (source, target) => {
            if (target) {
                target.value = source.value;
            }
        };

        const clearDynamicColumns = () => {
            rowDefinitions.forEach((def) => {
                const row = document.getElementById(def.rowId);
                while (row.children.length > 1) {
                    row.removeChild(row.lastElementChild);
                }
            });
        };

        const resetForm = () => {
            exerciseSection.reset();
            clearDynamicColumns();
            setCount = 0;
            formMode = 'create';
            exerciseNameCell.colSpan = 1;
            if (exerciseNameInput && exerciseNameMirror) {
                syncValue(exerciseNameInput, exerciseNameMirror);
            }
            if (deleteExerciseBtn) {
                deleteExerciseBtn.style.display = 'none';
                deleteExerciseBtn.disabled = true;
            }
        };

        const toggleForm = (show, options = {}) => {
            exerciseSection.style.display = show ? 'block' : 'none';
            addExerciseBtn.style.display = show ? 'none' : 'inline-flex';
            if (show) {
                formMode = options.mode || 'create';
                formTitle.textContent = options.title || (formMode === 'edit' ? 'Übung bearbeiten' : 'Neue Übung');
                if (exerciseIdInput) {
                    exerciseIdInput.value = options.id || '';
                }
                if (deleteExerciseBtn) {
                    const isEdit = formMode === 'edit';
                    deleteExerciseBtn.style.display = isEdit ? 'inline-flex' : 'none';
                    deleteExerciseBtn.disabled = !isEdit;
                }
            } else {
                resetForm();
            }
        };

        const buildInput = (index, def, preset) => {
            const attrs = [
                `name="sets[${index}][${def.name}]"`,
                `type="${def.type}"`,
                def.placeholder ? `placeholder="${escapeAttr(def.placeholder)}"` : '',
                def.min ? `min="${escapeAttr(def.min)}"` : '',
                def.max ? `max="${escapeAttr(def.max)}"` : '',
                def.step ? `step="${escapeAttr(def.step)}"` : '',
                def.maxLength ? `maxlength="${escapeAttr(def.maxLength)}"` : '',
                def.required ? 'required' : ''
            ].filter(Boolean).join(' ');

            let value;
            if (preset && Object.prototype.hasOwnProperty.call(preset, def.name)) {
                value = preset[def.name];
            } else if (typeof def.defaultValue === 'function') {
                value = def.defaultValue(index);
            } else if (def.defaultValue !== undefined) {
                value = def.defaultValue;
            } else {
                value = def.placeholder || '';
            }

            return `<input ${attrs} value="${escapeAttr(value)}">`;
        };

        const appendSetCells = (index, preset) => {
            rowDefinitions.forEach((def) => {
                const row = document.getElementById(def.rowId);
                const cell = document.createElement('td');
                cell.className = 'editable-cell';
                cell.innerHTML = buildInput(index, def, preset);
                row.appendChild(cell);
            });
        };

        const handleAddSet = (preset) => {
            if (setCount >= MAX_SETS) {
                setStatus(`Maximal ${MAX_SETS} Sets erreicht.`, true);
                return;
            }
            const index = setCount;
            setCount += 1;
            exerciseNameCell.colSpan = Math.max(setCount, 1);
            appendSetCells(index, preset);
        };

        const populateForm = (exercise) => {
            const nameValue = exercise && exercise.name ? exercise.name : '';
            exerciseNameInput.value = nameValue;
            if (exerciseNameMirror) {
                syncValue(exerciseNameInput, exerciseNameMirror);
            }
            const sets = exercise && Array.isArray(exercise.sets) ? exercise.sets : [];
            if (sets.length == 0) {
                handleAddSet();
                return;
            }
            sets.forEach((set) => {
                if (setCount >= MAX_SETS) {
                    return;
                }
                const repsValue = set && set.reps !== undefined ? Math.min(set.reps, MAX_REPS_PER_SET) : '';
                handleAddSet({
                    name: set && set.name !== undefined ? set.name : '',
                    reps: repsValue,
                    repDuration: set && set.repDuration !== undefined ? set.repDuration : '',
                    pauseBetween: set && set.pauseBetween !== undefined ? set.pauseBetween : '',
                    pauseAfter: set && set.pauseAfter !== undefined ? set.pauseAfter : '',
                    percentIntensity: set && set.percentIntensity !== undefined ? set.percentIntensity : ''
                });
            });
        };

        const openCreateForm = () => {
            resetForm();
            toggleForm(true, { mode: 'create', title: 'Neue Übung' });
            handleAddSet();
            if (exerciseNameInput) {
                exerciseNameInput.focus();
            }
        };

        const openEditForm = (exercise) => {
            resetForm();
            const idValue = exercise && exercise.id ? exercise.id : '';
            toggleForm(true, { mode: 'edit', title: 'Übung bearbeiten', id: idValue });
            populateForm(exercise);
            if (exerciseNameInput) {
                exerciseNameInput.focus();
            }
        };

        addExerciseBtn.addEventListener('click', openCreateForm);

        cancelButtons.forEach((btn) => {
            if (btn) {
                btn.addEventListener('click', () => toggleForm(false));
            }
        });

        addSetBtn.addEventListener('click', () => handleAddSet(), false);

        if (exerciseNameInput && exerciseNameMirror) {
            syncValue(exerciseNameInput, exerciseNameMirror);
            exerciseNameInput.addEventListener('input', () => syncValue(exerciseNameInput, exerciseNameMirror));
            exerciseNameMirror.addEventListener('input', () => syncValue(exerciseNameMirror, exerciseNameInput));
        }

        exerciseListContainer.addEventListener('click', (event) => {
            const target = event.target.closest('.exercise-item');
            if (!target) {
                return;
            }
            const id = target.getAttribute('data-id') || '';
            const exercise = state.exercises.find((item) => item && item.id === id);
            if (exercise) {
                openEditForm(exercise);
            }
        });

        exerciseSection.addEventListener('submit', async (event) => {
            event.preventDefault();
            if (setCount === 0) {
                handleAddSet();
                return;
            }

            const formData = new FormData(exerciseSection);
            const searchParams = new URLSearchParams();
            formData.forEach((value, key) => {
                searchParams.append(key, value);
            });

            try {
                const response = await fetch(`/submit?${searchParams.toString()}`, {
                    method: 'GET',
                    cache: 'no-store'
                });
                if (!response.ok) {
                    throw new Error(`HTTP ${response.status}`);
                }
                const payload = await response.json();
                if (!payload || payload.status !== 'ok') {
                    throw new Error('Speichern fehlgeschlagen.');
                }
                const wasEdit = formMode === 'edit';
                toggleForm(false);
                setStatus(wasEdit ? 'Übung aktualisiert.' : 'Übung gespeichert.', false);
                await fetchExercises();
            } catch (error) {
                console.error('Fehler beim Speichern', error);
                setStatus('Fehler beim Speichern.', true);
            }
        });

        if (deleteExerciseBtn) {
            deleteExerciseBtn.addEventListener('click', async () => {
                if (!exerciseIdInput || !exerciseIdInput.value) {
                    return;
                }
                const currentName = exerciseNameInput ? exerciseNameInput.value.trim() : '';
                const confirmText = currentName
                    ? `Übung "${currentName}" wirklich löschen?`
                    : 'Übung wirklich löschen?';
                if (!window.confirm(confirmText)) {
                    return;
                }

                try {
                    const response = await fetch(`/api/exercise?id=${encodeURIComponent(exerciseIdInput.value)}`, {
                        method: 'DELETE',
                        cache: 'no-store'
                    });
                    if (!response.ok) {
                        throw new Error(`HTTP ${response.status}`);
                    }
                    const payload = await response.json();
                    if (!payload || payload.status !== 'ok') {
                        throw new Error('Löschen fehlgeschlagen.');
                    }
                    toggleForm(false);
                    setStatus('Übung gelöscht.', false);
                    await fetchExercises();
                } catch (error) {
                    console.error('Fehler beim Löschen', error);
                    setStatus('Fehler beim Löschen.', true);
                }
            });
        }

        const fetchExercises = async () => {
            try {
                setStatus('Lade...', false);
                const response = await fetch('/api/exercises', { cache: 'no-store' });
                if (!response.ok) {
                    throw new Error(`HTTP ${response.status}`);
                }
                const payload = await response.json();
                const exercises = Array.isArray(payload.exercises) ? payload.exercises : [];
                state.exercises = exercises;
                if (exercises.length === 0) {
                    exerciseListContainer.innerHTML = '<div class="empty-state">Noch keine Übungen gespeichert.</div>';
                } else {
                    const html = exercises.map((exercise) => {
                        const safeId = exercise && exercise.id !== undefined ? exercise.id : '';
                        const safeName = escapeHtml(exercise && exercise.name ? exercise.name : 'Unbenannt');
                        const countValue = exercise && typeof exercise.setCount === 'number' ? exercise.setCount : 0;
                        const setMeta = `${countValue} Sets`;
                        return `<button class="exercise-item" data-id="${escapeAttr(safeId)}">
                                    <span class="exercise-item__name">${safeName}</span>
                                    <span class="exercise-item__meta">${escapeHtml(setMeta)}</span>
                                </button>`;
                    }).join('');
                    exerciseListContainer.innerHTML = html;
                }
                setStatus(exercises.length ? '' : 'Keine Übungen gespeichert.');
            } catch (error) {
                console.error('Fehler beim Laden der Übungen', error);
                exerciseListContainer.innerHTML = '<div class="empty-state">Fehler beim Laden der Übungen.</div>';
                setStatus('Fehler beim Laden.', true);
            }
        };

        fetchExercises();
    })();
    </script>
</body>
</html>
)rawliteral";

void handleRoot(WebServer& server) {
    server.send_P(200, "text/html", INDEX_PAGE);
}

} // namespace

WebService::WebService() : hasExercise_(0) {
    lastExerciseId_.fill(0);
}

const Exercise* WebService::lastExercise() const {
    return hasExercise_ ? storageService.findExercise(lastExerciseId_) : nullptr;
}

void WebService::registerRoutes(WebServer& server) {
    server.on("/", [&server]() { handleRoot(server); });
    server.on("/submit", [this, &server]() { this->handleSubmit(server); });
    server.on("/api/exercises", HTTP_GET, [this, &server]() { this->handleExercisesList(server); });
    server.on("/api/exercise", HTTP_GET, [this, &server]() { this->handleExerciseDetail(server); });
    server.on("/api/exercise", HTTP_DELETE, [this, &server]() { this->handleExerciseDelete(server); });
    server.on("/favicon.ico", HTTP_GET, [&server]() { server.send(204); });
    server.onNotFound([&server]() {
        Serial.printf("[Web] Unhandled request: %s\n", server.uri().c_str());
        server.send(404, "text/plain", "Not found");
    });
}

void WebService::handleExercisesList(WebServer& server) {
    String json = "{\"exercises\":[";
    const auto& records = storageService.exercises();
    for (size_t i = 0; i < records.size(); ++i) {
        if (i > 0) {
            json += ",";
        }
        json += exerciseRecordToJson(records[i]);
    }
    json += "]}";
    server.send(200, "application/json", json);
}

void WebService::handleExerciseDetail(WebServer& server) {
    const String idParam = server.arg("id");
    if (idParam.isEmpty()) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing id\"}");
        return;
    }
    if (idParam.length() != kExerciseIdHexLength) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid id\"}");
        return;
    }
    const auto id = StorageService::fromHex(idParam);
    const String normalized = StorageService::toHex(id);
    if (!idParam.equalsIgnoreCase(normalized)) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid id\"}");
        return;
    }

    if (const auto* record = storageService.findRecord(id)) {
        server.send(200, "application/json", exerciseRecordToJson(*record));
    } else {
        server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"Not found\"}");
    }
}

void WebService::handleExerciseDelete(WebServer& server) {
    const String idParam = server.arg("id");
    if (idParam.isEmpty()) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing id\"}");
        return;
    }
    if (idParam.length() != kExerciseIdHexLength) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid id\"}");
        return;
    }

    const auto id = StorageService::fromHex(idParam);
    const String normalized = StorageService::toHex(id);
    if (!idParam.equalsIgnoreCase(normalized)) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid id\"}");
        return;
    }

    if (!storageService.removeExercise(id)) {
        server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"Not found\"}");
        return;
    }

    storageService.savePersistent();

    if (hasExercise_ && id == lastExerciseId_) {
        hasExercise_ = 0;
        lastExerciseId_.fill(0);
    }

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebService::handleSubmit(WebServer& server) {
    Serial.println("[Web] Received exercise configuration:");
    for (uint8_t i = 0; i < server.args(); ++i) {
        Serial.printf("  %s = %s\n", server.argName(i).c_str(), server.arg(i).c_str());
    }

    const String exerciseNameParam = server.arg("exerciseName");
    std::string exerciseName = exerciseNameParam.c_str();

    const String idParam = server.arg("exerciseId");
    bool updateRequested = false;
    StorageService::ExerciseId updateId{};
    if (!idParam.isEmpty()) {
        if (idParam.length() != kExerciseIdHexLength) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid exercise id\"}");
            return;
        }
        updateId = StorageService::fromHex(idParam);
        const String normalized = StorageService::toHex(updateId);
        if (!idParam.equalsIgnoreCase(normalized)) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid exercise id\"}");
            return;
        }
        updateRequested = true;
    }

    std::map<int, SetInput> sets;

    for (uint8_t i = 0; i < server.args(); ++i) {
        const String argName = server.argName(i);
        if (!argName.startsWith("sets[")) {
            continue;
        }

        const int firstBracket = argName.indexOf('[');
        const int firstClose = argName.indexOf(']', firstBracket + 1);
        const int secondBracket = argName.indexOf('[', firstClose + 1);
        const int secondClose = argName.indexOf(']', secondBracket + 1);
        if (firstBracket < 0 || firstClose < 0 || secondBracket < 0 || secondClose < 0) {
            continue;
        }

        const int index = argName.substring(firstBracket + 1, firstClose).toInt();
        const String field = argName.substring(secondBracket + 1, secondClose);
        SetInput& set = sets[index];

        const String value = server.arg(i);
        if (field == "name") {
            set.name = value;
        } else if (field == "reps") {
            set.reps = value.toInt();
        } else if (field == "repDuration") {
            set.repDuration = value.toInt();
        } else if (field == "pauseBetween") {
            set.pauseBetween = value.toInt();
        } else if (field == "pauseAfter") {
            set.pauseAfter = value.toInt();
        } else if (field == "percentIntensity") {
            set.percentIntensity = value.toInt();
        }
    }

    if (!exerciseName.empty() && !sets.empty()) {
        if (exerciseName.length() > StorageService::kMaxExerciseNameLength) {
            exerciseName.resize(StorageService::kMaxExerciseNameLength);
        }

        Exercise builtExercise(exerciseName);
        size_t addedSets = 0;
        for (const auto& pair : sets) {
            if (addedSets >= StorageService::kMaxSets) {
                Serial.println("[Web] Set limit reached; remaining sets ignored.");
                break;
            }

            const SetInput& input = pair.second;
            Serial.printf("[Web] Set %d (%s): reps=%d repDuration=%d pauseBetween=%d pauseAfter=%d intensity=%d\n",
                          pair.first + 1,
                          input.name.c_str(),
                          input.reps,
                          input.repDuration,
                          input.pauseBetween,
                          input.pauseAfter,
                          input.percentIntensity);

            std::string setLabel = input.name.length() ? std::string(input.name.c_str())
                                                       : std::string("Set ") + std::to_string(pair.first + 1);
            if (setLabel.length() > StorageService::kMaxSetLabelLength) {
                setLabel.resize(StorageService::kMaxSetLabelLength);
            }

            Set set(std::move(setLabel), input.pauseAfter, input.percentIntensity);
            int clampedReps = std::max(0, std::min(input.reps, static_cast<int>(StorageService::kMaxRepsPerSet)));
            for (int r = 0; r < clampedReps; ++r) {
                set.reps.emplace_back(input.repDuration, input.pauseBetween);
            }

            builtExercise.sets.push_back(std::move(set));
            ++addedSets;
        }

        if (builtExercise.sets.empty()) {
            Serial.println("[Web] No sets after applying limits.");
        } else {
            StorageService::ExerciseId storedId{};
            bool stored = false;
            bool updated = false;

            if (updateRequested) {
                if (storageService.updateExercise(updateId, builtExercise)) {
                    storedId = updateId;
                    stored = true;
                    updated = true;
                    Serial.println("[Web] Exercise updated in memory.");
                }
            } else {
                if (storageService.addExercise(builtExercise, &storedId)) {
                    stored = true;
                    Serial.println("[Web] Exercise stored in memory.");
                }
            }

            if (stored) {
                storageService.savePersistent();
                lastExerciseId_ = storedId;
                hasExercise_ = 1;
                String response = "{\"status\":\"ok\",\"id\":\"";
                response += StorageService::toHex(storedId);
                response += "\",\"mode\":\"";
                response += updated ? "update" : "create";
                response += "\"}";
                server.send(200, "application/json", response);
                return;
            }
        }
    }

    if (!updateRequested) {
        hasExercise_ = 0;
    }
    Serial.println("[Web] Exercise data incomplete; nothing stored.");
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Ungültige Übung\"}");
}
