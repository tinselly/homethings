const HOST = "http://192.168.1.133:5000";

const Api = {
  getLedThings: () =>
    fetch(`${HOST}/things/led/list`, {
      headers: {
        "Access-Control-Allow-Origin": "*",
        "Access-Control-Allow-Methods": "GET, OPTIONS",
        "Access-Control-Request-Headers": "Content-Type",
      },
    }).then((r) => r.json()),

  setLedThing: (id, cmd) =>
    fetch(`${HOST}/things/led/${id}`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        "Access-Control-Allow-Origin": "*",
        "Access-Control-Allow-Methods": "GET, OPTIONS",
        "Access-Control-Request-Headers": "Content-Type",
      },
      body: JSON.stringify(cmd),
    }),
};

export default Api;
