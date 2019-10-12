import React, {Component} from "react";
import {connect} from "react-redux";
import Title from "../components/Title";
import Container from "@material-ui/core/Container";
import Grid from "@material-ui/core/Grid";
import TextField from "@material-ui/core/TextField";
import Button from "@material-ui/core/Button";
import {createLevel} from "../api/levels";
import {SHOW_MESSAGE} from "../redux/actions";

class CreateLevelPage extends Component {
    constructor(props) {
        super(props);
        this.onCreateLevelButtonClick = this.onCreateLevelButtonClick.bind(this);
    }

    async onCreateLevelButtonClick(title, map) {
        let response = await createLevel(title, map);
        if (! response.success) {
            this.props.showMessage(response.message);
            return;
        }
        let levelId = response.level.id;
    }

    render() {
        return <CreateLevelPageLayout onCreateLevelButtonClick={this.onCreateLevelButtonClick}/>
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

let titleInput;
let mapInput;

function CreateLevelPageLayout(props) {
    return (
        <Container maxWidth="lg">
            <Title>Create new level</Title>
            <form noValidate>
                <Grid container spacing={2}>
                    <Grid item xs={12} sm={6}>
                        <TextField
                            name="title"
                            variant="outlined"
                            required
                            fullWidth
                            id="title"
                            label="Title"
                            autoFocus
                            InputProps={{
                                inputRef: node => titleInput = node
                            }}
                        />
                    </Grid>
                    <Grid item xs={12}>
                        <TextField
                            name="map"
                            variant="outlined"
                            required
                            fullWidth
                            id="map"
                            label="Map"
                            InputProps={{
                                inputRef: node => mapInput = node
                            }}
                            helperText="Use . for empty cells and * for walls"
                        />
                    </Grid>
                    <Grid item xs={12}>
                        <Button size="large" variant="contained" color="primary" onClick={() => props.onCreateLevelButtonClick(titleInput.value, mapInput.value)}>
                            Create level
                        </Button>
                    </Grid>
                </Grid>

            </form>
        </Container>
    )
}

export default connect(CreateLevelPage.mapStateToProps, CreateLevelPage.mapDispatchToProps)(CreateLevelPage)