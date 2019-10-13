import React, {Component} from "react";
import {SHOW_MESSAGE} from "../redux/actions";
import {connect} from "react-redux";
import {Link, useParams} from "react-router-dom";
import {getLevel} from "../api/levels";
import LargeLoader from "../components/LargeLoader";
import Container from "@material-ui/core/Container";
import Title from "../components/Title";
import Grid from "@material-ui/core/Grid";
import {getProgram, runProgram} from "../api/programs";
import {Table, TableBody, TableHead, TextField} from "@material-ui/core";
import Typography from "@material-ui/core/Typography";
import Button from "@material-ui/core/Button";
import TableRow from "@material-ui/core/TableRow";
import TableCell from "@material-ui/core/TableCell";

function ProgramPageWrapper(props) {
    let params = useParams();
    return <ProgramPage params={params} {...props}/>;
}

class ProgramPage extends Component {
    constructor(props) {
        super(props);
        this.state = {
            isLoaded: false,
            level: {},
            program: {},
            params: [],
            runError: "",
            output: "",
            score: null,
        };
        this.onChangeParam = this.onChangeParam.bind(this);
        this.onRunProgram = this.onRunProgram.bind(this);
    }

    async componentDidMount() {
        let id = this.props.params.id;
        let response = await getLevel(id);
        if (! response.success) {
            this.props.showMessage(response.message ? response.message : "Could not load level");
            return;
        }
        let level = response.level;

        response = await getProgram(this.props.params.programId);
        if (! response.success) {
            this.props.showMessage(response.message ? response.message : "Could not load program");
            return;
        }
        let program = response.program;

        this.setState({
            isLoaded: true,
            level: level,
            program: program,
            params: [['', '']],
            runErrorMessage: "",
            output: "",
            score: null,
            success: null,
        });
    }

    onChangeParam(paramIndex, index, newValue) {
        this.setState(s => {
            let params = s.params;
            params[paramIndex][index] = newValue;
            if (paramIndex === params.length - 1)
                params.push(['', '']);
            return {...s, params: params}
        });
    }

    async onRunProgram(programId, params) {
        let paramsMap = {};
        params.forEach(({key, value}) => {
            paramsMap[key] = value;
        });
        let response = await runProgram(programId, paramsMap);
        if (! response.success) {
            this.props.showMessage(response.message ? response.message : "Can't run your program on drone");
            return;
        }
        if (response.error) {
            this.setState({
                runErrorMessage: response.errorMessage,
                output: "",
                score: null,
                success: null,
            });
            return;
        }

        this.setState({
            runErrorMessage: response.errorMessage,
            output: response.output,
            score: response.run.score,
            success: response.run.success,
        });
    }

    render() {
        if (! this.state.isLoaded)
            return <LargeLoader/>;
        return <ProgramPageLayout level={this.state.level} program={this.state.program} params={this.state.params} runErrorMessage={this.state.runErrorMessage} output={this.state.output}
                                  score={this.state.score} success={this.state.success}
                                  onChangeParam={this.onChangeParam} onRunProgram={this.onRunProgram}/>
    }

    static mapDispatchToProps(dispatch) {
        return {
            showMessage: (message) => dispatch({type: SHOW_MESSAGE, message: message}),
        }
    }
}

function ProgramPageLayout(props) {
    return (
        <Container maxWidth="lg">
            <Title>
                { props.program.title }
            </Title>
            <Grid container spacing={10}>
                <Grid item xs={12} md={6}>
                    <Typography gutterBottom paragraph>
                        Run drone with this program on level "{ props.level.title }" with following parameters:
                    </Typography>
                    <Table stickyHeader>
                        <TableHead>
                            <TableRow>
                                <TableCell>NAME</TableCell>
                                <TableCell>VALUE</TableCell>
                            </TableRow>
                        </TableHead>
                        <TableBody>
                        {
                            props.params.map((param, index) => (
                                <TableRow key={index}>
                                    <TableCell scope="row">
                                        <TextField
                                            value={param[0]}
                                            onChange={(event) => props.onChangeParam(index, 0, event.target.value)}
                                            label={"Name"}
                                        />
                                    </TableCell>
                                    <TableCell>
                                        <TextField
                                            value={param[1]}
                                            onChange={(event) => props.onChangeParam(index, 1, event.target.value)}
                                            label={"Value"}
                                        />
                                    </TableCell>
                                </TableRow>
                            ))
                        }
                        </TableBody>
                    </Table>
                </Grid>
                <Grid item xs={12} md={6}>
                    {
                        props.runErrorMessage &&
                        <Typography color="error">
                            Error on running your program: {props.runErrorMessage}
                        </Typography>
                    }
                    {
                        props.success === false &&
                            <Typography color="error" variant="h4">
                                Your drone did not reach the finish :-(
                            </Typography>
                    }
                    {
                        props.success === true &&
                            <React.Fragment>
                                <Typography color="textPrimary" variant="h4">
                                    Your program did { props.score} moves!
                                </Typography>
                                <Typography color="textSecondary" variant="body1" paragraph>
                                    Return back to <Link to={"/levels/" + props.level.id}>other programs</Link>.
                                </Typography>
                            </React.Fragment>
                    }
                    <Typography color="primary">
                        { props.output }
                    </Typography>
                </Grid>
                <Grid item xs={12}>
                    <Button size="large" variant="contained" color="primary" onClick={() => props.onRunProgram(props.program.id, props.params)}>
                        Run drone!
                    </Button>
                </Grid>
            </Grid>
        </Container>
    );
}

export default connect(ProgramPage.mapStateToProps, ProgramPage.mapDispatchToProps)(ProgramPageWrapper)