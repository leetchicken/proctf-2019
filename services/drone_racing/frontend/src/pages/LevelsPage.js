import React, {Component} from "react";
import {getLevels} from "../api/levels";
import {connect} from "react-redux";
import {SHOW_MESSAGE} from "../redux/actions";
import LargeLoader from "../components/LargeLoader";
import List from "@material-ui/core/List";
import {ListItemAvatar, ListItemText} from "@material-ui/core";
import ListItem from "@material-ui/core/ListItem";
import {Link} from "react-router-dom";
import AddIcon from "@material-ui/icons/Add"
import Avatar from "@material-ui/core/Avatar";
import Paper from "@material-ui/core/Paper";
import Container from "@material-ui/core/Container";
import Title from "../components/Title";

class LevelsPage extends Component {
    constructor(props) {
        super(props);
        this.state = {
            isLoaded: false,
            levels: [],
        }
    }

    async componentDidMount() {
        let response = await getLevels();
        if (! response.success) {
            this.props.showMessage(response.message ? response.message : "Could not load levels, try later");
            return;
        }
        this.setState({
            isLoaded: true,
            levels: response.levels
        });
    }

    render() {
        if (! this.state.isLoaded)
            return <LargeLoader/>;
        return (
            <LevelsPageLayout levels={this.state.levels} />
        )
    }

    static mapStateToProps(state) {
        return {

        }
    }

    static mapDispatchToProps(dispatch) {
        return {
            showMessage: (message) => dispatch({type: SHOW_MESSAGE, message: message}),
        }
    }
}

function LevelsPageLayout(props) {
    return (
        <Container maxWidth="lg">
            <Title>Levels</Title>
            <List component="nav" aria-label="levels list">
                <ListItem button component={Link} to="/levels/new">
                    <ListItemAvatar>
                        <Avatar>
                            <AddIcon/>
                        </Avatar>
                    </ListItemAvatar>
                    <ListItemText secondary="Create your own level for drone racing">Add new level</ListItemText>
                </ListItem>
                {
                    props.levels.map(level => (
                        <ListItem key={level.id} button component={Link} to={"/levels/" + level.id}>
                            <ListItemText>{level.title}</ListItemText>
                        </ListItem>
                    ))
                }
            </List>
        </Container>
    );
}

export default connect(LevelsPage.mapStateToProps, LevelsPage.mapDispatchToProps)(LevelsPage)