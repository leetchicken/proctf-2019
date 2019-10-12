import {LOGIN, LOGOUT, SHOW_MESSAGE} from "./actions"

export const initialState = {
    is_authenticated: false,
    user: {},
    message: ""
};

export function reduce(state = initialState, action) {
    switch (action.type) {
        case LOGIN:
            return {
                ...state,
                is_authenticated: true,
                user: action.user,
            };
        case LOGOUT:
            return {
                ...state,
                is_authenticated: false,
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