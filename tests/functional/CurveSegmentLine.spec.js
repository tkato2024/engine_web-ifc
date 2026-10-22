const { test } = require("@jest/globals");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const { IfcAPI, LogLevel } = require(process.env.WEB_IFC_API || "../../dist/web-ifc-api-node.js");

// IfcCurveSegments trimming an IfcLine whose IfcVector has magnitude 2, standalone and inside an
// IfcCompositeCurve. A parameter length of 3 spans 3 * 2 = 6 m; a length measure of 3 spans 3 m.
const fixture = path.join(__dirname, "fixtures", "curve-segment-line", "vector-magnitude.ifc");
const cases = [
  [61, "standalone, parameter length", 6],
  [62, "standalone, length measure", 3],
  [63, "composite, parameter length", 6],
  [64, "composite, length measure", 3],
];

async function length(expressID) {
  const api = new IfcAPI();
  await api.Init();
  api.SetLogLevel(LogLevel.LOG_LEVEL_OFF);
  const model = api.OpenModel(new Uint8Array(fs.readFileSync(fixture)));
  try {
    const mesh = api.GetFlatMesh(model, expressID);
    const placed = mesh.geometries.get(0);
    const g = api.GetGeometry(model, placed.geometryExpressID);
    const v = api.GetVertexArray(g.GetVertexData(), g.GetVertexDataSize());
    const last = v.length - 6;
    const result = Math.hypot(v[last] - v[0], v[last + 1] - v[1], v[last + 2] - v[2]);
    g.delete();
    return result;
  } finally {
    api.CloseModel(model);
  }
}

for (const [expressID, label, expected] of cases) {
  test(`curve segment line: ${label}`, async () => {
    const actual = await length(expressID);
    assert.ok(Math.abs(actual - expected) < 1e-6, `length ${actual}, expected ${expected}`);
  });
}
