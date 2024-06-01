const SERVER_PORT = 3000;

const express = require("express");
const app = express();

const jsonMiddleware = express.json();
app.use(jsonMiddleware);

const formidable = require("formidable");

app.listen(SERVER_PORT, () => {
  console.log(`Server started on port ${SERVER_PORT}`);
});
