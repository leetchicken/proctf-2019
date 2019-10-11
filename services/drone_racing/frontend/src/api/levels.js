import {get, post} from "./http";

export async function create_level(title, map) {
    let response = await post("/api/levels", {
        title: title,
        map: map
    });
    return response ? response["level"] : false;
}

export async function get_levels() {
    let response = await get("/api/levels");
    return response ? response["levels"] : false;
}