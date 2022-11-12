import { useEffect, useState } from "react";

import Row from "react-bootstrap/Row";
import Col from "react-bootstrap/Col";
import Container from "react-bootstrap/Container";
import ThemeProvider from "react-bootstrap/ThemeProvider";
import Button from "react-bootstrap/Button";
import ListGroup from "react-bootstrap/ListGroup";
import ButtonGroup from "react-bootstrap/ButtonGroup";
import Accordion from "react-bootstrap/Accordion";

import LedThing from "./LedThing";
import Api from "./Api";
import Utils from "./Utils";

import logo from "./images/ufo.png";

import "./App.css";

function App() {
  const [ledThings, setLedThings] = useState([]);

  const [icon] = useState(Utils.randomIcon());

  useEffect(() => {
    const init = () => Api.getLedThings().then((data) => setLedThings(data));

    init();

    const interval = setInterval(init, 15000);

    return () => clearInterval(interval);
  }, []);

  return (
    <ThemeProvider>
      <Container fluid>
        <Row className="justify-content-center">
          <img
            src={logo}
            style={{
              width: "25vh",
            }}
          />
        </Row>
        <Row className="justify-content-center">
          <h1>{`HomeThings ${icon}`}</h1>
        </Row>
        <br />
        {ledThings.map((ledThing) => (
          <LedThing
            id={ledThing.id}
            key={ledThing.id}
            initialColors={ledThing.colors}
            anim={ledThing.animation}
            intensity={ledThing.intensity}
          />
        ))}
      </Container>
    </ThemeProvider>
  );
}

export default App;
