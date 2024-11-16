const express = require('express');
const path = require('path');
const app = express();
const port = 80;

// Serve the HTML page
app.use(express.static(path.join(__dirname, 'data')));

// Mock ToF data endpoint
app.get('/tof_data', (req, res) => {
  const tofData = Array.from({ length: 64 }, () => Math.floor(Math.random() * 1500)); // Generate random values
  res.json(tofData);
});

app.listen(port, () => {
  console.log(`Server is running on http://localhost:${port}`);
});
