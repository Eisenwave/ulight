"use strict";

const fs = require("fs");

const indent = "    ";

function generateCss(theme) {
    let css = "";

    for (const mode of ["light", "dark"]) {
        const mediaQuery = mode === "light"
            ? "@media (prefers-color-scheme: light) {\n"
            : "@media not (prefers-color-scheme: light) {\n";
        css += mediaQuery;

        const sortedEntries = Object.entries(theme[mode])
            .sort(([k0, _0], [k1, _1]) => k0.localeCompare(k1));

        for (const [key, value] of sortedEntries) {
            const declarations = [];
            if (typeof value === "string") {
                declarations.push(`color: ${value};`);
            } else if (typeof value === "object") {
                if (value.color !== undefined) {
                    declarations.push(`color: ${value.color};`);
                }
                if (value["font-style"] !== undefined) {
                    declarations.push(`font-style: ${value["font-style"]};`);
                }
            }

            const cssSelector = `${indent}h-[data-h^=${key.replace(/-/g, "_")}]`;
            const cssBlock = `{\n${declarations.map(d => `${indent.repeat(2)}${d}`).join("\n")}\n${indent}}`
            css += `${cssSelector} ${cssBlock}\n`;
        }
        css += "}\n\n";
    }
    return css;
}

function main() {
    if (process.argv.length < 3) {
        console.error(`Usage: ${process.argv[0]} <input.json> [output.css]`);
        process.exit(1);
    }

    const inputFile = process.argv[2];
    const outputFile = process.argv[3];

    try {
        const jsonData = fs.readFileSync(inputFile, "utf8");
        const theme = JSON.parse(jsonData);

        const css = generateCss(theme);
        if (outputFile !== undefined) {
            fs.writeFileSync(outputFile, css, "utf8");
        } else {
            process.stdout.write(css);
        }
    } catch (error) {
        console.error("Error processing file:", error);
    }
}

main();
