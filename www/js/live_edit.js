import {
    container,
    codeInput,
    editorContentsItem,
    editorFractionItem,
    editorFractionLimit,
    isEditorVertical,
    resizeContainerToFraction,
    setEditorVertical,
    languageIdSelect,
    setLanguageId,
    getLanguageId,
    setTheme,
    themePicker
} from "./live_edit_core.js";

import {
    loadWasm
} from "./ulight.js";

const whileDraggingClass = 'while-dragging';

const separator = document.getElementById('separator');
const splitVerticalButton = document.getElementById('button-split-vertical');
const splitHorizontalButton = document.getElementById('button-split-horizontal');
const inputLineNumbers = document.getElementById('input-line-numbers');
const outputLineNumbers = document.getElementById('output-line-numbers');
const codeHighlight = document.getElementById('code-highlight');
const output = document.getElementById('output');

/**
 * Determines the fraction (horizontal or vertical) of the cursor position.
 * @param {DOMRect} rect the rectangle
 * @param {MouseEvent} e the mouse event
 * @param {boolean} vertical if true, calculates the fraction based on y/height
 * @returns {number} a fraction between `0` and `1`
 */
function determineDragFraction(rect, e, vertical = false) {
    if (vertical) {
        const offsetY = e.clientY - rect.top;
        return offsetY / rect.height;
    } else {
        const offsetX = e.clientX - rect.left;
        return offsetX / rect.width;
    }
}

/**
 * @param {HTMLElement} e
 * @param {number} n
 */
function setVisibleLineNumbers(e, n) {
    const arr = Array.from({ length: n }, (_, i) => i + 1);
    e.textContent = arr.join('\n');
}

function onCodeInput(persist) {
    if (persist) {
        localStorage.setItem(editorContentsItem, codeInput.value);
    }

    try {
        const languageId = Number(languageIdSelect.value);
        const highlightedInput = wasm.toHtml(codeInput.value, languageId);
        setVisibleLineNumbers(inputLineNumbers, highlightedInput.count('\n') + 1);
        codeHighlight.innerHTML = highlightedInput;

        const highlightedOutput = wasm.toHtml(highlightedInput, "html");
        output.innerHTML = highlightedOutput;
        setVisibleLineNumbers(outputLineNumbers, highlightedOutput.count('\n') + 1);
    } catch (e) {
        setVisibleLineNumbers(inputLineNumbers, codeInput.value.count('\n') + 1);
        codeHighlight.textContent = codeInput.value;
        output.textContent = `Error: ${e.message}\n\n${e.stack}`;
        return;
    }
}

function setLanguageOptions() {
    const map = new Map();
    const langs = wasm.getLanguageList();
    for (const { displayName, id } of langs) {
        map.set(id, displayName);
    }
    for (const [name, displayName] of map.entries()) {
        const option = document.createElement("option");
        option.value = name;
        option.textContent = displayName;
        languageIdSelect.appendChild(option);
    }
    // Weird hack necessary to deal with the fact that the <select>
    // element just now got filled with values,
    // which makes the usual UI restore from localStorage note work.
    setLanguageId(getLanguageId(), false);
}

const indent = '    ';

let isDragging = false;

// =================================================================================================

/**
 * @param {number} min the lower bound
 * @param {number} max the upper bound
 * @returns {number}
 */
Number.prototype.clamp = function (min, max) {
    return Math.min(Math.max(this, min), max);
};

/**
 * Returns the number of occurrences of `x` within this string.
 * @param {string} x the character to count
 * @returns {number}
 */
String.prototype.count = function (x) {
    let n = 0;
    for (const c of this) {
        if (x === c) {
            ++n;
        }
    }
    return n;
};

/**
 * Insert a string into this string at a given index.
 * @param {string} str the string to insert
 * @param {number} index the index at which to insert, where `0` means prepending before the string
 * @returns {string}
 */
String.prototype.insertAt = function (str, index) {
    return this.substring(0, index) + str + this.substring(index);
}

String.prototype.removeAt = function (amount, index) {
    return this.substring(0, index) + this.substring(index + amount);
}

String.prototype.indexNotOf = function (c, position) {
    for (let i = position ?? 0; i < this.length; ++i) {
        if (!c.includes(this[i])) {
            return i;
        }
    }
    return -1;
}

String.prototype.lastIndexNotOf = function (c, position) {
    for (let i = position ?? this.length - 1; i >= 0; --i) {
        if (!c.includes(this[i])) {
            return i;
        }
    }
    return -1;
}

codeInput.addEventListener('keydown', (e) => {
    if (e.key === 'Tab') {
        e.preventDefault();
        const start = codeInput.selectionStart;
        const end = codeInput.selectionEnd;

        const beforeLineStart = codeInput.value.lastIndexOf('\n', Math.max(0, start - 1));
        if (e.shiftKey) {
            const firstNonIndent = codeInput.value.indexNotOf(' ', beforeLineStart + 1);
            let indentEnd = firstNonIndent < 0 ? start : firstNonIndent;
            const currentIndentLength
                = indentEnd - (beforeLineStart + 1);
            if (currentIndentLength === 0) {
                return;
            }
            const removalSize = Math.min(currentIndentLength, indent.length);
            codeInput.value = codeInput.value.removeAt(removalSize, beforeLineStart + 1);
            codeInput.setSelectionRange(start - removalSize, end - removalSize);
        } else {
            // this is actually correct even if '\n' couldn't be found and -1 is returned
            codeInput.value = codeInput.value.insertAt(indent, beforeLineStart + 1);
            codeInput.setSelectionRange(start + indent.length, end + indent.length);
        }
        codeInput.dispatchEvent(new InputEvent('input', {
            'bubbles': true,
            'cancelable': false
        }));
    }
    else if (e.key === 'Enter') {
        const start = codeInput.selectionStart;
        const end = codeInput.selectionEnd;
        if (e.shiftKey || start !== end) {
            return;
        }

        e.preventDefault();
        const beforeLineStart = codeInput.value.lastIndexOf('\n', Math.max(0, start - 1));
        const firstNonIndent = codeInput.value.indexNotOf(' \t', beforeLineStart + 1);
        const indentEnd = firstNonIndent < 0 ? start : firstNonIndent;
        const currentIndent = codeInput.value.substring(beforeLineStart + 1, indentEnd);

        const previousNonWhitespace = codeInput.value.lastIndexNotOf(' \t\r', Math.max(0, start - 1));
        const newScopeIndent = previousNonWhitespace !== -1 && codeInput.value[previousNonWhitespace] === '{'
            ? indent : '';

        const addedIndent = currentIndent + newScopeIndent;
        const addedLength = addedIndent.length + 1;

        codeInput.value = codeInput.value.insertAt('\n' + addedIndent, start);
        codeInput.setSelectionRange(start + addedLength, start + addedLength);
        codeInput.dispatchEvent(new InputEvent('input', {
            'bubbles': true,
            'cancelable': false
        }));
    }
});

separator.addEventListener('mousedown', (e) => {
    isDragging = true;
    document.body.classList.add(whileDraggingClass);
});

document.addEventListener('mousemove', (e) => {
    if (!isDragging) {
        return;
    }

    const f = determineDragFraction(container.getBoundingClientRect(), e, isEditorVertical())
        .clamp(editorFractionLimit, 1 - editorFractionLimit);

    resizeContainerToFraction(f);
    localStorage.setItem(editorFractionItem, f);
});

document.addEventListener('mouseup', () => {
    if (isDragging) {
        isDragging = false;
        document.body.classList.remove(whileDraggingClass)
    }
});

const persistOrientationChanges = true;

splitVerticalButton.addEventListener('click', (e) => {
    e.preventDefault();
    setEditorVertical(true, persistOrientationChanges);
});

splitHorizontalButton.addEventListener('click', (e) => {
    e.preventDefault();
    setEditorVertical(false, persistOrientationChanges);
});

// =================================================================================================

const wasm = await loadWasm();

codeInput.addEventListener('input', () => onCodeInput(true));
languageIdSelect.addEventListener('change', () => onCodeInput(true));
languageIdSelect.addEventListener('input', () => setLanguageId(languageIdSelect.value, true));
themePicker.addEventListener('input', () => setTheme(themePicker.value, true));

setLanguageOptions();
onCodeInput(false);

