"use strict";

const fs = require("fs");
const yargs = require('yargs');

const indent = "    ";

const longNameToShort = {
    "error": "err",
    "comment": "cmt",
    "comment-delim": "cmt_dlim",
    "value": "val",
    "number": "num",
    "number-delim": "num_dlim",
    "number-decor": "num_deco",
    "string": "str",
    "string-delim": "str_dlim",
    "string-decor": "str_deco",
    "escape": "esc",
    "null": "null",
    "bool": "bool",
    "this": "this",
    "macro": "mac",
    "id": "id",
    "id-decl": "id_decl",
    "id-var": "id_var",
    "id-var-decl": "id_var_decl",
    "id-const": "id_cons",
    "id-const-decl": "id_cons_decl",
    "id-function": "id_fun",
    "id-function-decl": "id_fun_decl",
    "id-type": "id_type",
    "id-type-decl": "id_type_decl",
    "id-module": "id_mod",
    "id-module-decl": "id_mod_decl",
    "id-label": "id_labl",
    "id-label-decl": "id_labl_decl",
    "id-parameter": "id_para",
    "id-argument": "id_arg",
    "keyword": "kw",
    "keyword-control": "kw_ctrl",
    "keyword-type": "kw_type",
    "keyword-op": "kw_op",
    "attr": "attr",
    "attr-delim": "attr_dlim",
    "diff-heading": "diff_head",
    "diff-heading-hunk": "diff_head_hunk",
    "diff-common": "diff_eq",
    "diff-deletion": "diff_del",
    "diff-insertion": "diff_ins",
    "diff-modification": "diff_mod",
    "markup-tag": "mk_tag",
    "markup-attr": "mk_attr",
    "markup-deletion": "mk_del",
    "markup-insertion": "mk_ins",
    "markup-emph": "mk_emph",
    "markup-strong": "mk_stro",
    "markup-emph-strong": "mk_emph_stro",
    "markup-underline": "mk_ulin",
    "markup-emph-underline": "mk_emph_ulin",
    "markup-strong-underline": "mk_stro_ulin",
    "markup-emph-strong-underline": "mk_emph_stro_ulin",
    "markup-strikethrough": "mk_strk",
    "markup-emph-strikethrough": "mk_emph_strk",
    "markup-strong-strikethrough": "mk_stro_strk",
    "markup-emph-strong-strikethrough": "mk_emph_stro_strk",
    "markup-underline-strikethrough": "mk_ulin_strk",
    "markup-emph-underline-strikethrough": "mk_emph_ulin_strk",
    "markup-strong-underline-strikethrough": "mk_stro_ulin_strk",
    "markup-emph-strong-underline-strikethrough": "mk_emph_stro_ulin_strk",
    "sym": "sym",
    "sym-punc": "sym_punc",
    "sym-parens": "sym_par",
    "sym-square": "sym_sqr",
    "sym-brace": "sym_brac",
    "sym-op": "sym_op",
    "shell-command": "sh_cmd",
    "shell-command-builtin": "sh_cmd_bltn",
    "shell-option": "sh-opt"
};

const variants = ["light", "dark"];

function generateCss(path, theme, themeName = "", noBlockBackground = false) {
    let css = "";

    let firstVariant = true;
    for (const [variant, data] of Object.entries(theme)) {
        if (variant === "meta") {
            continue;
        }
        if (!variants.includes(variant)) {
            console.error(`${path}: Unknown variant "${variant}"`);
            process.exit(1);
        }
        if (firstVariant) {
            firstVariant = false;
        } else {
            css += "\n\n";
        }

        if (themeName.length === 0) {
            const mediaQuery = variant === "light"
                ? "@media (prefers-color-scheme: light) {\n"
                : "@media (prefers-color-scheme: dark) {\n";
            css += mediaQuery;
        }
        const themePrefix = themeName.length !== 0 ?
            `[data-ulight-theme=${variant}-${themeName}]` : "";

        const sortedEntries = Object.entries(data)
            .sort(([k0, _0], [k1, _1]) => compareKeys(k0, k1));

        for (const [key, value] of sortedEntries) {
            if (key === "background") {
                if (typeof value !== "string") {
                    console.error(`${path}: The value of "background" may only be a string.`);
                    process.exit(1);
                }
                if (noBlockBackground) {
                    continue;
                }
            }
            const indentLevel = themeName.length !== 0 ? 0 : 1;
            const cssSelector = `${indent.repeat(indentLevel)}${keyToCssSelector(path, key, themePrefix)}`;
            const declarations = jsonValueToCssDeclarations(path, key, value);
            const cssBlock = cssDeclarationsInBlock(declarations, indentLevel);
            css += `${cssSelector} ${cssBlock}\n`;
        }
        if (themeName.length === 0) {
            css += "}";
        }
    }
    return css;
}

const specialKeys = ["background", "foreground"];

Array.prototype.indexOfOr = function (item, defaultIndex) {
    const result = this.indexOf(item);
    return result === -1 ? defaultIndex : result;
}

function compareKeys(x, y) {
    const xi = specialKeys.indexOfOr(x, specialKeys.length);
    const yi = specialKeys.indexOfOr(y, specialKeys.length);
    return xi - yi !== 0 ? xi - yi : x.localeCompare(y);
}

function keyToCssSelector(path, key, themePrefix) {
    if (key === "foreground") {
        return `${themePrefix}.ulight-code, ${themePrefix}.ulight-code-block`;
    }
    if (key === "background") {
        return `${themePrefix}.ulight-code-block`;
    }

    const shortName = longNameToShort[key];
    if (shortName === undefined) {
        console.error(`${path}: Invalid highlight type "${key}"`);
        process.exit(1);
    }

    const fullPrefix = themePrefix.length === 0 ? "" : themePrefix + " ";
    return `${fullPrefix}h-[data-h^=${shortName}]`;
}

function jsonValueToCssDeclarations(path, key, value) {
    if (typeof value === "string") {
        if (value.length === 0) {
            console.error(`${path}: Empty string value for key "${key}" is not allowed.`);
            process.exit(1);
        }
        return key === "background" ?
            [`background-color: ${value};`, `border-color: ${value};`]
            : [`color: ${value};`];
    }
    return Object.entries(value)
        .map(([property, value]) => `${property}: ${value};`);
}

function cssDeclarationsInBlock(declarations, indentLevel) {
    const joined = declarations
        .map(d => `${indent.repeat(indentLevel + 1)}${d}`)
        .join("\n");
    return `{\n${joined}\n${indent.repeat(indentLevel)}}`;
}

function filePathToThemeName(path) {
    const pattern = /(?:.*[/\\])*(.*(?=\.[^.]*)|.*)/;
    return path.match(pattern)[1].replace('_', '-');
}

function fileToCss(path, isThemed, noBlockBackground) {
    const themeName = isThemed ? filePathToThemeName(path) : "";
    if (themeName.includes(".")) {
        console.error(`${path}: Unable to generate theme name from input file "${path}"`);
        process.exit(1);
    }
    const jsonData = fs.readFileSync(path, "utf8");
    const theme = JSON.parse(jsonData);

    return generateCss(path, theme, themeName, noBlockBackground);
}

function main() {
    const argv = yargs
        .option("input", {
            alias: "i",
            describe: "Input file/directory",
            type: "string",
            demandOption: true
        })
        .option("output", {
            alias: "o",
            describe: "Output file",
            type: "string",
            demandOption: false
        })
        .options("themed", {
            alias: "t",
            describe: "Emit CSS for selectable theming",
            type: "boolean"
        })
        .options("no-block-background", {
            alias: "B",
            describe: "Ignore code block background",
            type: "boolean"
        })
        .argv;

    const isThemed = argv.themed ?? false;
    const noBlockBackground = argv["no-block-background"] ?? false;

    const inputStats = fs.lstatSync(argv.input);
    const files = inputStats.isDirectory() ?
        fs.readdirSync(argv.input).map(f => `${argv.input}/${f}`) : [argv.input];

    try {
        const css = files.map(f => fileToCss(f, isThemed, noBlockBackground))
            .join("\n");
        if (argv.output !== undefined) {
            fs.writeFileSync(argv.output, css, "utf8");
        } else {
            process.stdout.write(css);
        }
    } catch (error) {
        console.error("Error processing file:", error);
        process.exit(1);
    }
}

main();
