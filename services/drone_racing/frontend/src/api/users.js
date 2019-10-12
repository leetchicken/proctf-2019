import {get, post} from "./http";

export async function isAuthenticated() {
    return await get("/api/users/me");
}

export async function authenticate(login, password) {
    return await post("/api/users/login", {
        login: login,
        password: password
    });
}

export async function register(name, login, password) {
    return await post("/api/users", {
        name: name,
        login: login,
        password: password,
    });
}

export async function logout() {
    return await post("/api/users/logout");
}