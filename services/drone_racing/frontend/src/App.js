import React, { Component } from 'react';
import './App.css';
import {isAuthenticated, authenticate, logout} from "./api/users";
import {
  BrowserRouter as Router,
  Switch,
  Route,
  Link,
  useParams,
} from "react-router-dom";
import LoginPage from "./pages/LoginPage";
import RegisterPage from "./pages/RegisterPage";
import {connect} from "react-redux";
import Snackbar from "@material-ui/core/Snackbar";
import IconButton from "@material-ui/core/IconButton";
import CloseIcon from '@material-ui/icons/Close';
import {LOGIN, LOGOUT, SHOW_MESSAGE} from "./redux/actions";
import LevelsPage from "./pages/LevelsPage";
import LargeLoader from "./components/LargeLoader";
import Header from "./components/Header";
import CreateLevelPage from "./pages/CreateLevelPage";
import LevelPage from "./pages/LevelPage";
import CreateProgramPage from "./pages/CreateProgramPage";
import ProgramPage from "./pages/ProgramPage";

class App extends Component {
  constructor(props) {
    super(props);
    this.state = {
      isLoaded: false,
    };
    this.onLoginButtonClick = this.onLoginButtonClick.bind(this);
    this.onLogoutButtonClick = this.onLogoutButtonClick.bind(this);
    this.onCloseSnackbar = this.onCloseSnackbar.bind(this);
  }

  async componentDidMount() {
    let response = await isAuthenticated();
    if (response.success) {
      this.props.setUser(response.user);
    }
    this.setState({
      isLoaded: true,
    });
  }

  async onLoginButtonClick(login, password) {
    let response = await authenticate(login, password);
    if (! response.success) {
      this.props.showMessage(response.message);
      return;
    }
    this.props.setUser(response.user);
  }

  async onLogoutButtonClick() {
    let response = await logout();
    if (! response.success) {
      this.props.showMessage("Can't log out");
      return;
    }
    this.props.logout()
  }

  onCloseSnackbar() {
    this.props.clearMessage();
  }

  render() {
    if (!this.state.isLoaded)
      return <LargeLoader/>

    return (
        <React.Fragment>
          {this.props.is_authenticated && <Header user={this.props.user} onLogoutButtonClick={this.onLogoutButtonClick}/>}
          <Router>
            {!this.props.is_authenticated && this.renderLayoutForNonAuthenticated()}
            {this.props.is_authenticated && this.renderLayoutForAuthenticated()}
          </Router>
          <Snackbar
              anchorOrigin={{
                vertical: 'bottom',
                horizontal: 'right',
              }}
              open={this.props.message !== ""}
              autoHideDuration={2000}
              onClose={this.onCloseSnackbar}
              ContentProps={{
                'aria-describedby': 'message-id',
              }}
              message={<span id="message-id">{this.props.message}</span>}
              action={[
                <IconButton
                    key="close"
                    aria-label="close"
                    color="inherit"
                    onClick={this.onCloseSnackbar}
                >
                  <CloseIcon />
                </IconButton>,
              ]}
          />
        </React.Fragment>
    );
  }

  renderLayoutForNonAuthenticated() {
    return (<Switch>
      <Route exact path="/users/register">
        <RegisterPage/>
      </Route>
      <Route exact path="/users/login">
        <LoginPage onLoginButtonClick={this.onLoginButtonClick}/>
      </Route>
      <Route>
        <LoginPage onLoginButtonClick={this.onLoginButtonClick}/>
      </Route>
    </Switch>);
  }

  renderLayoutForAuthenticated() {
    return (
      <main style={{"paddingTop": "32px", "paddingBottom": "32px"}}>
        <Switch>
          <Route exact path="/levels">
            <LevelsPage />
          </Route>
          <Route exact path="/levels/new">
            <CreateLevelPage />
          </Route>
          <Route exact path="/levels/:id">
            <LevelPage />
          </Route>
          <Route exact path="/levels/:id/program">
            <CreateProgramPage />
          </Route>
          <Route exact path="/levels/:id/programs/:programId">
            <ProgramPage />
          </Route>
          <Route>
            <LevelsPage />
          </Route>
       </Switch>
      </main>
    )
  }

  static mapStateToProps(state) {
    return {
      is_authenticated: state.is_authenticated,
      user: state.user,
      message: state.message,
    }
  }

  static mapDispatchToProps(dispatch) {
    return {
      showMessage: message => dispatch({type: SHOW_MESSAGE, message: message}),
      clearMessage: () => dispatch({type: SHOW_MESSAGE, message: ''}),
      setUser: user => dispatch({type: LOGIN, user: user}),
      logout: () => dispatch({type: LOGOUT}),
    }
  }
}

export default connect(App.mapStateToProps, App.mapDispatchToProps)(App);
