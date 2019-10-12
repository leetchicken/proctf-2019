import {makeStyles} from "@material-ui/core";
import CircularProgress from "@material-ui/core/CircularProgress";
import React from "react";

export default function LargeLoader() {
    const classes = makeStyles(theme => ({
        container: {
            display: 'flex',
            justifyContent: 'center',
            margin: 'auto',
            alignItems: 'center',
            width: '100%',
            height: '100%',
        },
    }))();

    return <div className={classes.container}>
        <CircularProgress/>
    </div>
}