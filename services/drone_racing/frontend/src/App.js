import React, { Component } from 'react';
import './App.css';
import { is_authenticated, authenticate } from "./api/users";
import {
  BrowserRouter as Router,
  Switch,
  Route,
  Link
} from "react-router-dom";
import LoginPage from "./pages/LoginPage";
import RegisterPage from "./pages/RegisterPage";

class App extends Component {
  constructor(props) {
    super(props);
    this.state = {
      is_loaded: false,
      is_authenticated: false,
    };
    this.login = this.login.bind(this)
  }

  async componentDidMount() {
    let success = await is_authenticated();
    this.setState({
      is_loaded: true,
      is_authenticated: success
    })

  }

  async login() {
    let login = document.getElementsByName("login")[0].value;
    let password = document.getElementsByName("password")[0].value;
    let success = await authenticate(login, password);
    this.setState({
      is_authenticated: success
    })
  }

  render() {
    return (
        <Router>
            <Switch>
              <Route exact path="/users/login">
                <LoginPage />
              </Route>
              <Route exact path="/users/register">
                <RegisterPage />
              </Route>
            </Switch>
        </Router>
    );
    // if (this.state.is_loaded) {
    //   return (
    //       <LoginPage/>
    //   )
    //   // return (
    //   //     <div>
    //   //       Is authenticated: { this.state.is_authenticated ? "true": "false" }
    //   //       <input type="text" name="login"/>
    //   //       <input type="text" name="password"/>
    //   //       <Button onClick={this.login} variant="contained" color="primary">Login</Button>
    //   //     </div>
    //   // )
    // }
    // return (
    //     <div className="App">
    //       <header className="App-header">
    //         <p>
    //           Edit <code>src/App.js</code> and save to reload.
    //         </p>
    //         <a
    //             className="App-link"
    //             href="https://reactjs.org"
    //             target="_blank"
    //             rel="noopener noreferrer"
    //         >
    //           Learn React 222
    //         </a>
    //       </header>
    //     </div>
    // );
  }
}

export default App;
