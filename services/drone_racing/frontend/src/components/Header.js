import React from 'react';
import { makeStyles } from '@material-ui/core/styles';
import AppBar from '@material-ui/core/AppBar';
import Toolbar from '@material-ui/core/Toolbar';
import Typography from '@material-ui/core/Typography';
import Button from '@material-ui/core/Button';
import {AccountCircle} from "@material-ui/icons";
import {Link} from "react-router-dom";

const useStyles = makeStyles(theme => ({
    root: {
        flexGrow: 1,
    },
    menuButton: {
        color: 'white',
        marginRight: theme.spacing(2),
    },
    usernameContainer: {
        display: 'inline-flex',
        justifyContent: 'center',
        alignContent: 'center',
        alignItems: 'center',
        padding: '8px 11px',
        textTransform: 'uppercase',
        boxSizing: 'border-box',
        verticalAlign: 'middle',
        fontSize: '0.9375rem',
    },
    username: {
        fontWeight: '500',
        paddingLeft: '8px',
    },
    title: {
        flexGrow: 1,
    },
    titleLink: {
        color: 'inherit',
        textDecoration: 'none',
        cursor: 'pointer',
        fontSize: 'inherit',
        fontWeight: 'inherit',
    }
}));

export default function Header(props) {
    const classes = useStyles();

    return (
        <div className={classes.root}>
            <AppBar position="static">
                <Toolbar>
                    <Typography variant="h6" className={classes.title}>
                        <Link to="/" className={classes.titleLink}>Drone racing</Link>
                    </Typography>
                    <div style={{"position": "relative"}}>
                        <div className={classes.usernameContainer}>
                            <AccountCircle/>
                            <div className={classes.username}>
                                { props.user.name }
                            </div>
                        </div>

                        <Button
                            size="large"
                            className={classes.menuButton}
                            onClick={props.onLogoutButtonClick}
                        >
                            Logout
                        </Button>
                    </div>

                </Toolbar>
            </AppBar>
        </div>
    );
}