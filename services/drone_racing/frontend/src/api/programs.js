import {get, post} from "./http"

export async function createProgram(levelId, title, sourceCode) {
    return await post("/api/programs", {
        levelId: levelId,
        title: title,
        sourceCode: sourceCode,
    });
}

export async function getMyPrograms(levelId) {
    return await get("/api/programs?level_id=" + levelId);
}

export async function getProgram(programId) {
    return await get("/api/programs/" + programId)
}

export async function runProgram(programId, params) {
    return await post("/api/runs", {
        programId: programId,
        params: params,
    });
}

export async function getLevelRuns(levelId) {
    return await get("/api/runs?level_id=" + levelId);
}