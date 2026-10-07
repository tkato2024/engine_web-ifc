// Run after npm run gen-schema: node -r ts-node/register tests/schema-generator/type-metadata.ts
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {parseElements, parseExpressType, resolveExpressType, walkParents, crc32, makeCRCTable} from "../../src/schema-generator/gen_functional_types_helpers";
import {ExpressType} from "../../src/schema-generator/gen_functional_types_interfaces";

const root = resolve(__dirname, "../..");
const header = readFileSync(resolve(root, "src/cpp/web-ifc/schema/schema-type-metadata.h"), "utf8");
const crcTable = makeCRCTable();

for (const schema of ["IFC2X3", "IFC4", "IFC4X3"]) {
    const {types, entities} = parseElements(readFileSync(resolve(root, `src/schema-generator/${schema}.exp`), "utf8"));
    const resolveType = (declaration: ExpressType) => resolveExpressType(declaration, types, entities);
    const named = (name: string) => resolveType(parseExpressType(name));
    for (const type of types) assert.notEqual(resolveType(type.expressType).baseType, "UNKNOWN", `${schema}.${type.name}`);
    for (const entity of entities) {
        walkParents(entity, entities);
        for (const prop of entity.derivedProps) assert.notEqual(resolveType(prop.expressType).baseType, "UNKNOWN", `${schema}.${entity.name}.${prop.name}`);
        assert.ok(header.includes(`std::array<ExpressTypeMetadata, ${entity.derivedProps.length}> kAttributeTypes_${schema}_${entity.name} =`));
        assert.ok(header.includes(`{${crc32(entity.name.toUpperCase(), crcTable)}, kAttributeTypes_${schema}_${entity.name}}`));
    }
    assert.ok(header.includes(`std::array<NamedTypeMetadata, ${types.length}> kNamedTypes_${schema}`));
    assert.ok(header.includes(`std::array<EntityTypeMetadata, ${entities.length}> kEntityTypes_${schema}`));
    assert.ok(header.includes(`{${schema}, kNamedTypes_${schema}, kEntityTypes_${schema}}`));
    assert.deepEqual(named("IfcPositiveLengthMeasure"), {baseType: "REAL", aggregates: []});
    assert.deepEqual(named("IfcComplexNumber"), {baseType: "REAL", aggregates: ["ARRAY"]});
    assert.deepEqual(named("IfcCompoundPlaneAngleMeasure"), {baseType: "INTEGER", aggregates: ["LIST"]});
    assert.equal(named("IfcCountMeasure").baseType, schema === "IFC4X3" ? "INTEGER" : "NUMBER");
    assert.equal(named("IfcBoolean").baseType, "BOOLEAN");
    assert.equal(named("IfcLogical").baseType, "LOGICAL");
    assert.equal(named("IfcLabel").baseType, "STRING");
    assert.equal(named("IfcUnitEnum").baseType, "ENUM");
    assert.equal(named("IfcValue").baseType, "SELECT");
    assert.equal(named("IfcWall").baseType, "ENTITY");
    assert.equal(named("IfcDoesNotExist").baseType, "UNKNOWN");
    assert.equal(named("IfcBinary").baseType, schema === "IFC2X3" ? "UNKNOWN" : "BINARY");
    const exponent = entities.find(entity => entity.name === "IfcDimensionalExponents")!.props[0];
    assert.equal(exponent.expressType.typeName, "INTEGER");
    assert.equal(resolveType(exponent.expressType).baseType, "INTEGER");
    if (schema !== "IFC2X3") {
        const controlPoints = entities.find(entity => entity.name === "IfcBSplineSurface")!.props.find(prop => prop.name === "ControlPointsList")!;
        assert.deepEqual(resolveType(controlPoints.expressType), {baseType: "ENTITY", aggregates: ["LIST", "LIST"]});
        assert.ok(header.includes(`kAggregates_${schema}_LIST_LIST = {AggregateKind::LIST, AggregateKind::LIST}`));
    }
    const wall = entities.find(entity => entity.name === "IfcWall")!;
    assert.equal(wall.derivedProps[0].name, "GlobalId");
    assert.equal(resolveType(wall.derivedProps[0].expressType).baseType, "STRING");
    for (const [entityName, propName, baseType, aggregate] of [
        ["IfcCartesianPoint", "Coordinates", "REAL", "LIST"],
        ["IfcQuantityLength", "LengthValue", "REAL", ""],
        ["IfcBSplineCurve", "ClosedCurve", "LOGICAL", ""],
        ["IfcCompositeCurveSegment", "SameSense", "BOOLEAN", ""]
    ]) {
        const entity = entities.find(entity => entity.name === entityName)!;
        const position = entity.derivedProps.findIndex(prop => prop.name === propName);
        assert.ok(position >= 0);
        assert.deepEqual(resolveType(entity.derivedProps[position].expressType), {baseType, aggregates: aggregate ? [aggregate] : []});
        const array = header.split(`kAttributeTypes_${schema}_${entityName} = {{`)[1].split("}};")[0];
        const entries = array.match(/\{ExpressBaseType::\w+, (?:std::span<const AggregateKind>\{\}|kAggregates_\w+)\}/g)!;
        assert.equal(entries[position], `{ExpressBaseType::${baseType}, ${aggregate ? `kAggregates_${schema}_${aggregate}` : "std::span<const AggregateKind>{}"}}`);
    }
    const countTable = header.split(`kNamedTypes_${schema} = {{`)[1].split("}};")[0];
    assert.ok(countTable.includes(`{${crc32("IFCCOUNTMEASURE", crcTable)}, {ExpressBaseType::${schema === "IFC4X3" ? "INTEGER" : "NUMBER"}, std::span<const AggregateKind>{}}}`));
}

const fixture = parseElements("TYPE Inner = ARRAY [1:2] OF REAL; END_TYPE; TYPE Alias = Inner; END_TYPE; TYPE Outer = LIST [1:?] OF Alias; END_TYPE; TYPE CycleA = CycleB; END_TYPE; TYPE CycleB = CycleA; END_TYPE;");
assert.deepEqual(resolveExpressType(parseExpressType("SET [1:?] OF Outer"), fixture.types, fixture.entities), {baseType: "REAL", aggregates: ["SET", "LIST", "ARRAY"]});
assert.equal(resolveExpressType(parseExpressType("CycleA"), fixture.types, fixture.entities).baseType, "UNKNOWN");
assert.deepEqual(parseExpressType("OPTIONAL LIST [1:?] OF UNIQUE ARRAY [1:2] OF OPTIONAL STRING(8)"), {typeName: "STRING", aggregates: ["LIST", "ARRAY"]});
assert.deepEqual(parseExpressType("BAG [0:?] OF INTEGER"), {typeName: "INTEGER", aggregates: ["BAG"]});
console.log("EXPRESS type metadata checks passed for all three schemas.");
