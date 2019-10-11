import {get, post} from "./http"

export async function create_program(level_id, title, source_code) {
    let response = await post("/api/programs", {
        levelId: level_id,
        title: title,
        sourceCode: source_code,
    });
    return response ? response["programId"] : false;
}

export async function get_my_programs(level_id) {
    let response = await get("/api/programs?level_id=" + level_id);
    return response ? response["programs"] : false;
}

export async function run_program(program_id, params) {
    let response = await post("/api/runs", {
        programId: program_id,
        params: params,
    });
    return response;
}

export async function get_level_runs(level_id) {
    let response = await get("/api/runs?level_id=" + level_id);
    return response ? response["runs"] : false;
}