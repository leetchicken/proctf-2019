import {get, post} from "./http";

export async function createLevel(title, map) {
    return await post("/api/levels", {
        title: title,
        map: map
    });
}

export async function getLevels() {
    return await get("/api/levels");
}

export async function getLevel(levelId) {
    return await get("/api/levels/" + levelId)
}