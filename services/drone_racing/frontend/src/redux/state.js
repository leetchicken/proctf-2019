import {LOGIN, LOGOUT, SHOW_MESSAGE} from "./actions"

export const initialState = {
    isAuthenticated: false,
    user: {},
    message: ""
};

export function reduce(state = initialState, action) {
    switch (action.type) {
        case LOGIN:
            return {
                ...state,
                isAuthenticated: true,
                user: action.user,
            };
        case LOGOUT:
            return {
                ...state,
                isAuthenticated: false,
                user: {},
            };
        case SHOW_MESSAGE:
            return {
                ...state,
                message: action.message,
            };
        default:
            return state
    }
}