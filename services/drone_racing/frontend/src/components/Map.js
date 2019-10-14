import React, {Component} from "react";
import {makeStyles} from "@material-ui/core";
import clsx from "clsx";

class Map extends Component {
    constructor(props) {
        super(props);
        let size = Math.sqrt(props.map.length);
        let cells = [[]];
        for (let i = 0; i < props.map.length; i++) {
            cells[cells.length - 1].push(props.map[i] === '*');
            if (i % size === size - 1)
                cells.push([]);
        }
        this.state = {
            size: size,
            map: cells,
        };
    }

    render() {
        return <MapLayout size={this.state.size} map={this.state.map} />
    }
}

const useStyles = makeStyles(theme => ({
    map: {
        marginLeft: '10px',
        marginBottom: '30px',
    },
    row: {
        display: 'block',
        '&:first-child > div': {
            borderTop: '1px solid #ccc',
        },
        '&:first-child > :first-child': {
            backgroundColor: 'green',
            borderColor: 'green',
            borderTopColor: '#ccc',
        },
        '&:last-child > :last-child': {
            backgroundColor: 'red',
            borderColor: 'red',
        },
    },
    cell: {
        display: 'inline-block',
        width: '40px',
        height: '40px',
        borderRight: '1px solid #ccc',
        borderBottom: '1px solid #ccc',
        '&:first-child': {
            borderLeft: '1px solid #ccc',
        }
    },
    small: {
        width: '20px',
        height: '20px',
    },
    wall: {
        backgroundColor: 'grey',
        borderColor: 'grey',
    },
}));

function MapLayout(props) {
    let classes = useStyles();
    let result = [];
    for (let i = 0; i < props.size; i++) {
        let row = [];
        for (let j = 0; j < props.size; j++)
            row.push(<Cell wall={props.map[i][j]} small={props.size >= 20} key={i + " " + j}/>);
        result.push(<Row key={i}>{ row }</Row>);
    }
    return <div className={classes.map}>{ result }</div>;
}

function Row(props) {
    let classes = useStyles();
    return <div className={classes.row}>{ props.children }</div>;
}

function Cell(props) {
    let classes = useStyles();
    let className = clsx(classes.cell, props.wall && classes.wall, props.small && classes.small);
    return <div className={className}>&nbsp;</div>
}

export default Map;