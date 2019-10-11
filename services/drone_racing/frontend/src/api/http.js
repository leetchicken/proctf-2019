export async function get(url) {
    let response = await fetch(url, {
        headers: {
            "Accept": "application/json",
        },
    });

    return await processResponse(response, url);
}

export async function post(url, body) {
    let response = await fetch(url, {
        headers: {
            "Accept": "application/json",
            "Content-Type": "application/json"
        },
        method: "POST",
        body: JSON.stringify(body)
    });

    return await processResponse(response, url);
}

async function processResponse(response, url) {
    if (response.status === 401 && !url.startsWith("/api/users")) {
        // Not authenticated
        console.log('Not authenticated')
    }

    response = await response.json();

    if (response["status"] === "error") {
        // Show bubble with response["message"]
        return false;
    }

    return response["response"];
}

