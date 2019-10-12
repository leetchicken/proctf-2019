import React from 'react';
import Typography from '@material-ui/core/Typography';
import {makeStyles} from "@material-ui/core";

const useStyles = makeStyles(theme => ({
    title: {
        padding: '10px',
    }
}));

export default function Title(props) {
    let classes = useStyles();
    return (
        <Typography component="h2" variant="h4"  gutterBottom className={classes.title}>
            {props.children}
        </Typography>
    );
}