const DEF_EXTRACTION_RESULTS_PATH = "./results/";
const REMOVE_TIMEOUT_MS = 10000;
const SERVER_PORT = 3000;

const express = require("express");
const app = express();

const formidable = require("formidable");

app.use(express.json()); // JSON middleware

app.post("/get-png-chunks", (req, res) => {
  const form = new formidable.IncomingForm();

  form.parse(req, (err, fields, files) => {
    if (err) {
      res.json({ code: err.httpCode, message: err.message });
    } else {
      const spawn = require("child_process").spawn;
      const process = spawn("../extractor/getPngChunks.exe", [
        files["image"][0].filepath,
      ]);

      process.stderr.on("data", (error) => {
        console.log(error.toString());
        process.on("close", (code) => {
          if (code != 0) {
            console.log(`Process status code is ${code}`);
            res.sendStatus(500);
          }
        });
      });

      // Data contains name of JSON, which contains chunks info
      process.stdout.on("data", (data) => {
        const resultsPath = `${DEF_EXTRACTION_RESULTS_PATH}${data}`;
        const extractionResults = require(resultsPath);
        // Remove 10sec later
        const fs = require("fs");
        setTimeout(() => {
          fs.unlink(resultsPath.toString(), (err) => {
            if (err) throw err;
          });
        }, REMOVE_TIMEOUT_MS);

        res.status(200).json(extractionResults);
        return;
      });
    }
  });
});

app.listen(SERVER_PORT, () => {
  console.log(`Server started on port ${SERVER_PORT}`);
});
