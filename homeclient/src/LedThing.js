import { useEffect, useState } from "react";

import Row from "react-bootstrap/Row";
import Col from "react-bootstrap/Col";
import Container from "react-bootstrap/Container";
import Button from "react-bootstrap/Button";
import ListGroup from "react-bootstrap/ListGroup";
import ButtonGroup from "react-bootstrap/ButtonGroup";

import { HexColorPicker, HexAlphaColorPicker } from "react-colorful";

import Utils from "./Utils";
import Api from "./Api";

const syncLed = Utils.debounce((id, cmd) => Api.setLedThing(id, cmd));

function LedThing({ id, initialColors = [], onSync = () => {} }) {
  const [colors, setColors] = useState(initialColors);
  const [colorIndex, setColorIndex] = useState(0);

  useEffect(() => {
    syncLed(id, {
      colors: colors.map((color) => (color.length < "#FFFFFFFF".length) ? color + "FF" : color),
    });
  }, [id, colors]);

  const changeColor = (newColor) => {
    setColors(colors.map((color, i) => (i === colorIndex ? newColor : color)));
  };

  const addColor = (color) => {
    setColors(colors.concat([color]));
  };

  const removeColor = () => {
    setColors(colors.filter((color, i) => i + 1 < colors.length));
  };

  return (
    <Container fluid>
      <Row className="justify-content-center">
        <h2>{"🛸  " + id.toUpperCase()}</h2>
      </Row>
      <Row className="justify-content-center">
        <HexAlphaColorPicker color={colors[colorIndex]} onChange={changeColor} />
      </Row>
      <Row className="p-4">
        <ListGroup>
          {colors.map((color, i) => (
            <ListGroup.Item
              style={{
                backgroundColor: "#57128a",
                color: "#FFFFFF",
              }}
              onClick={() => setColorIndex(i)}
              active={i === colorIndex}
            >
              <Row className="align-items-center">
                <Col ms="8">{`Color #${i + 1}`} </Col>
                <Col>
                  <div
                    style={{
                      backgroundColor: color,
                      height: "32px",
                      borderRadius: "16px",
                    }}
                  />
                </Col>
              </Row>
            </ListGroup.Item>
          ))}
        </ListGroup>
      </Row>
  
      <Row className="justify-content-right p-2">
        <ButtonGroup aria-label="Basic example">
          <Button
            variant="outline-warning"
            onClick={() => addColor("#ff00ff")}
            disabled={colors.length >= 10}
          >
            +
          </Button>
          <Button
            variant="outline-warning"
            onClick={() => removeColor()}
            disabled={colors.length <= 2}
          >
            -
          </Button>
        </ButtonGroup>
      </Row>
      <Row className="justify-content-center p-4"></Row>
    </Container>
  );
}

export default LedThing;
