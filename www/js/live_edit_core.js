export const container = document.getElementById('container');
export const codeInputContainer = document.getElementById('code-input-container');
export const codeInput = document.getElementById('code-input');
export const codeInputLayers = document.getElementById('code-input-layers');
export const outputContainer = document.getElementById('output-container');
export const output = document.getElementById('output');
export const languageIdSelect = document.getElementById('language-id');
export const themePicker = document.getElementById('theme-picker');

export const editorFractionLimit = 0.1;
export const editorFractionItem = 'editorFraction';
export const editorIsVerticalItem = 'editorIsVertical';
export const editorContentsItem = 'editorContents';
export const languageIdIndexItem = 'languageId';
export const themeItem = 'theme';

let isVertical = false;

export function resizeContainerToFraction(f) {
    const value = `${f}fr var(--separator-width) ${1 - f}fr`;
    if (isVertical) {
        container.style.gridTemplateRows = value;
    } else {
        container.style.gridTemplateColumns = value;
    }
}

/**
 * Swaps the `gridTemplateColumns` and `gridTemplateRows` in a style.
 * @param {CSSStyleDeclaration} e 
 */
function swapGridTemplateRowsColumns(e) {
    let temp = e.gridTemplateColumns;
    e.gridTemplateColumns = e.gridTemplateRows;
    e.gridTemplateRows = temp;
}

export function isEditorVertical() {
    return isVertical;
}

export function setEditorVertical(vertical, persist = false) {
    if (vertical) {
        document.body.classList.add('vertical');
    } else {
        document.body.classList.remove('vertical');
    }
    if (isVertical == vertical) {
        return;
    }
    swapGridTemplateRowsColumns(container.style);
    isVertical = vertical;
    if (persist) {
        localStorage.setItem(editorIsVerticalItem, vertical);
    }
}

export function setEditorContents(value, persist = false, dispatch = false) {
    codeInput.textContent = value;
    if (persist) {
        localStorage.setItem(editorContentsItem, value);
    }
    if (dispatch) {
        codeInput.dispatchEvent(new InputEvent('input', {
            'bubbles': true,
            'cancelable': false
        }));
    }
}

let languageId = 1;

export function getLanguageId() {
    return languageId;
}

export function setLanguageId(id, persist = false) {
    languageId = id;
    languageIdSelect.value = id;
    if (persist) {
        localStorage.setItem(languageIdIndexItem, id);
    }
}

export function setTheme(theme, persist = false) {
    themePicker.value = theme;
    codeInputContainer.setAttribute('data-ulight-theme', theme);
    outputContainer.setAttribute('data-ulight-theme', theme);
    if (persist) {
        localStorage.setItem(themeItem, theme);
    }
}

const initialFraction = localStorage.getItem(editorFractionItem);
if (initialFraction !== null) {
    resizeContainerToFraction(Number(initialFraction), false);
}

const initialIsVertical = localStorage.getItem(editorIsVerticalItem);
if (initialIsVertical !== null) {
    setEditorVertical(initialIsVertical === 'true', false);
}

const initialContents = localStorage.getItem(editorContentsItem);
if (initialContents !== null) {
    setEditorContents(initialContents, false, true);
}

const initialLanguageId = localStorage.getItem(languageIdIndexItem);
if (initialLanguageId !== null) {
    setLanguageId(Number(initialLanguageId), false);
}

const initialTheme = localStorage.getItem(themeItem);
if (initialTheme !== null) {
    setTheme(initialTheme, false);
}
