import React, {Component} from "react";
import {connect} from "react-redux";
import Title from "../components/Title";
import Container from "@material-ui/core/Container";
import Grid from "@material-ui/core/Grid";
import TextField from "@material-ui/core/TextField";
import Button from "@material-ui/core/Button";
import {getLevel} from "../api/levels";
import {SHOW_MESSAGE} from "../redux/actions";
import {useParams, withRouter} from "react-router-dom"
import LargeLoader from "../components/LargeLoader";
import {createProgram} from "../api/programs";
import {Typography} from "@material-ui/core";

function CreateProgramPageWrapper(props) {
    let params = useParams();
    let ComponentWithConnect = connect(CreateProgramPage.mapStateToProps, CreateProgramPage.mapDispatchToProps)(CreateProgramPage);
    let ComponentWithRouter = withRouter(({ history }) => (<ComponentWithConnect params={params} history={history} {...props}/>));
    return <ComponentWithRouter/>
}

class CreateProgramPage extends Component {
    constructor(props) {
        super(props);
        this.state = {
            isLoaded: false,
            level: {},
            compilationError: "",
        };
        this.onCreateProgramButtonClick = this.onCreateProgramButtonClick.bind(this);
    }

    async componentDidMount() {
        let id = this.props.params.id;
        let response = await getLevel(id);
        if (! response.success) {
            this.props.showMessage(response.message ? response.message : "Could not load level");
            return;
        }
        let level = response.level;
        this.setState({
            isLoaded: true,
            level: level,
        })
    }

    async onCreateProgramButtonClick(levelId, title, sourceCode) {
        let response = await createProgram(levelId, title, sourceCode);
        if (! response.success) {
            this.setState({
                compilationError: response.message,
            });
            return;
        }
        let programId = response.programId;
        this.props.history.push("/levels/" + levelId);
    }

    render() {
        if (!this.state.isLoaded)
            return <LargeLoader/>;
        return <CreateProgramPageLayout level={this.state.level} compilationError={this.state.compilationError} onCreateProgramButtonClick={this.onCreateProgramButtonClick}/>
    }

    static mapDispatchToProps(dispatch) {
        return {
            showMessage: (message) => dispatch({type: SHOW_MESSAGE, message: message}),
        }
    }
}

let titleInput;
let sourceCodeInput;

function CreateProgramPageLayout(props) {
    return (
        <Container maxWidth="lg">
            <Title>Upload drone program for level "{ props.level.title }"</Title>
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
                            helperText="Just for scoreboard"
                        />
                    </Grid>
                </Grid>
                <Grid container spacing={2}>
                    <Grid item xs={12} md={6}>
                        <TextField
                            name="source_code"
                            variant="outlined"
                            required
                            fullWidth
                            id="source_Code"
                            label="Source Code"
                            multiline
                            rows={20}
                            InputProps={{
                                inputRef: node => sourceCodeInput = node
                            }}
                            helperText="Use if statements, for loops, functions and other features from our Drone Language!"
                        />
                    </Grid>
                    <Grid item xs={12} md={6}>
                        <Typography color="error" paragraph>
                            { props.compilationError }
                        </Typography>
                    </Grid>
                    <Grid item xs={12}>
                        <Button size="large" variant="contained" color="primary" onClick={() => props.onCreateProgramButtonClick(props.level.id, titleInput.value, sourceCodeInput.value)}>
                            Save program
                        </Button>
                    </Grid>
                </Grid>
            </form>
        </Container>
    )
}

export default CreateProgramPageWrapper;