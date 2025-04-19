function testTemplateLiterals() {
    const name = "User";
    const greeting = `Hello, ${name}!`;

    const complex = `Value: ${`Nested ${2 + 3} template`}`;

    const obj = { name: "Item", price: 9.99 };
    const description = `${obj.name} costs $${obj.price.toFixed(2)}`;

    const formatted = `Result: ${(() => {
        const x = 10;
        return x * 2;
    })()}`;

    const isAdmin = true;
    const message = `You are ${isAdmin ? `an ${"admin"}` : "a user"}`;

    const complex2 = `${obj.name} / ${obj.price} ${`nested with " and ' quotes`} ${obj["complex key"]}`;
    return { greeting, complex, description, formatted, message, complex2 };
}
