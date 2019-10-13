import React, {Component} from "react";
import {SHOW_MESSAGE} from "../redux/actions";
import {connect} from "react-redux";
import {Link, useParams} from "react-router-dom";
import {getLevel} from "../api/levels";
import LargeLoader from "../components/LargeLoader";
import Container from "@material-ui/core/Container";
import Title from "../components/Title";
import Grid from "@material-ui/core/Grid";
import {getLevelRuns, getMyPrograms} from "../api/programs";
import List from "@material-ui/core/List";
import ListItem from "@material-ui/core/ListItem";
import {ListItemAvatar, ListItemText, Table, TableBody, TableHead} from "@material-ui/core";
import Avatar from "@material-ui/core/Avatar";
import AddIcon from "@material-ui/icons/Add";
import Typography from "@material-ui/core/Typography";
import Button from "@material-ui/core/Button";
import TableRow from "@material-ui/core/TableRow";
import TableCell from "@material-ui/core/TableCell";
import Map from "../components/Map";

function LevelPageWrapper(props) {
    let params = useParams();
    return <LevelPage params={params} {...props}/>;
}

class LevelPage extends Component {
    constructor(props) {
        super(props);
        this.state = {
            isLoaded: false,
            level: {},
            programs: [],
            runs: [],
        }
    }

    async componentDidMount() {
        let id = this.props.params.id;
        let response = await getLevel(id);
        if (! response.success) {
            this.props.showMessage(response.message ? response.message : "Could not load level");
            return;
        }
        let level = response.level;

        response = await getMyPrograms(id);
        if (! response.success) {
            this.props.showMessage(response.message ? response.message : "Could not load programs");
            return;
        }
        let programs = response.programs;

        response = await getLevelRuns(id);
        if (! response.success) {
            this.props.showMessage(response.message ? response.message : "Could not load runs");
            return;
        }
        let runs = response.runs.sort((a, b) => a.score - b.score);

        this.setState({
            isLoaded: true,
            level: level,
            programs: programs,
            runs: runs,
        });
    }

    render() {
        if (! this.state.isLoaded)
            return <LargeLoader/>;
        return <LevelPageLayout level={this.state.level} programs={this.state.programs} runs={this.state.runs}/>
    }

    static mapDispatchToProps(dispatch) {
        return {
            showMessage: (message) => dispatch({type: SHOW_MESSAGE, message: message}),
        }
    }
}

function LevelPageLayout(props) {
    return (
        <Container maxWidth="lg">
            <Title>{ props.level.title }</Title>
            <Map map={ props.level.map } />
            <Grid container spacing={10}>
                <Grid item xs={12} md={6}>
                    <Title>Your drone programs</Title>
                    <List>
                        <ListItem button component={Link} to={"/levels/" + props.level.id + "/program"}>
                            <ListItemAvatar>
                                <Avatar>
                                    <AddIcon/>
                                </Avatar>
                            </ListItemAvatar>
                            <ListItemText secondary="Run your drone on this level!">Add new program</ListItemText>
                        </ListItem>
                        {
                            props.programs.map(program => (
                                <ListItem key={program.id}>
                                    <ListItemText>
                                        {program.title}
                                    </ListItemText>
                                    <Button color={"primary"} component={Link} to={"/levels/" + props.level.id + "/programs/" + program.id}>
                                        View and run
                                    </Button>
                                </ListItem>
                            ))
                        }
                    </List>
                </Grid>
                <Grid item xs={12} md={6}>
                    <Title>Scoreboard</Title>
                    {
                        props.runs.length > 0 &&
                        <Table stickyHeader>
                            <TableHead>
                                <TableRow>
                                    <TableCell>AUTHOR</TableCell>
                                    <TableCell>PROGRAM</TableCell>
                                    <TableCell align="right">MOVES</TableCell>
                                </TableRow>
                            </TableHead>
                            <TableBody>
                            {
                                props.runs.map(run => (
                                    <TableRow key={run.id}>
                                        <TableCell>
                                            {run.program.author.name}
                                        </TableCell>
                                        <TableCell scope="row">
                                            {run.program.title}
                                        </TableCell>
                                        <TableCell align="right">
                                            {run.score}
                                        </TableCell>
                                    </TableRow>
                                ))
                            }
                            </TableBody>
                        </Table>
                    }
                    {
                        props.runs.length === 0 &&
                        <Typography color="textSecondary">
                            No runs for a while. Be the first!
                        </Typography>
                    }
                </Grid>
            </Grid>
        </Container>
    );
}

export default connect(null, LevelPage.mapDispatchToProps)(LevelPageWrapper)