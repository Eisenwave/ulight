function TestComponent() {
    const count = 5;
    const items = ['Apple', 'Banana', 'Cherry'];

    return (
        <div className="container">
            <h1>Hello JSX</h1>
            <p>Count: {count}</p>

            {count > 0 && (
                <p>You have {count} item{count !== 1 && 's'}</p>
            )}

            <button onClick={() => handleClick(count)}>
                Click me {count} time{count !== 1 && 's'}
            </button>

            <ul>
                {items.map((item, index) => (
                    <li key={index}>
                        {index + 1}: {item.toUpperCase()}
                        {item.length > 5 && <span className="long-item">*</span>}
                    </li>
                ))}
            </ul>

            <div>
                {isLoggedIn ? (
                    <UserProfile user={{ name: "John", role: `admin${getLevel()}` }} />
                ) : (
                    <LoginForm message={`Please login ${time ? `(${time})` : ""}`} />
                )}
            </div>

            <p data-test={`literal-${count * 2}-test`}>
                {`This is a ${template} literal inside JSX with ${nestedValue ? `nested ${nestedValue}` : "fallback"}`}
            </p>
        </div>
    );
}
