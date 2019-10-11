import {get, post} from "./http";

export async function is_authenticated() {
    let response = await get("/api/users/me");
    return !!response;
}

export async function authenticate(login, password) {
    let response = await post("/api/users/login", {
        login: login,
        password: password
    });
    return !!response;
}

export async function register(name, login, password) {
    let response = await post("/api/users", {
        name: name,
        login: login,
        password: password,
    });
    return !!response;
}