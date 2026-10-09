// Run after npm run gen-schema: node -r ts-node/register tests/schema-generator/type-metadata.ts
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {parseElements, parseExpressType, resolveExpressType, resolveSelectCandidates, walkParents, crc32, makeCRCTable} from "../../src/schema-generator/gen_functional_types_helpers";
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
    assert.equal(named("IfcValue").resolvedTypeName, "IfcValue");
    assert.equal(named("IfcWall").resolvedTypeName, "IfcWall");
    assert.equal(named("IfcLabel").resolvedTypeName, undefined);
    assert.equal(named("IfcDoesNotExist").resolvedTypeName, undefined);
    assert.equal(named("IfcDoesNotExist").baseType, "UNKNOWN");
    assert.equal(named("IfcBinary").baseType, schema === "IFC2X3" ? "UNKNOWN" : "BINARY");
    const select = (name: string) => resolveSelectCandidates(name, types, entities);
    const selects = types.filter(type => type.isSelect);
    assert.ok(header.includes(`std::array<SelectTypeMetadata, ${selects.length}> kSelectTypes_${schema}`));
    assert.ok(header.includes(`{${schema}, kSelectTypes_${schema}}`));
    for (const type of selects) {
        const candidates = select(type.name);
        assert.ok(candidates.length > 0, `${schema}.${type.name}`);
        assert.equal(new Set(candidates).size, candidates.length);
        for (const name of candidates) assert.notEqual(named(name).baseType, "SELECT");
        const arrayName = `kSelectCandidates_${schema}_${type.name}`;
        assert.ok(header.includes(`std::array<uint32_t, ${candidates.length}> ${arrayName} = {${candidates.map(name => crc32(name.toUpperCase(), crcTable)).join(", ")}};`));
        assert.ok(header.includes(`{${crc32(type.name.toUpperCase(), crcTable)}, ${arrayName}}`));
    }
    assert.deepEqual(select("IfcActorSelect"), ["IfcOrganization", "IfcPerson", "IfcPersonAndOrganization"]);
    assert.deepEqual(select("IfcTrimmingSelect"), ["IfcCartesianPoint", "IfcParameterValue"]);
    assert.deepEqual(select("IfcUnit"), schema === "IFC2X3"
        ? ["IfcDerivedUnit", "IfcNamedUnit", "IfcMonetaryUnit"]
        : ["IfcDerivedUnit", "IfcMonetaryUnit", "IfcNamedUnit"]);
    assert.ok(!select("IfcUnit").includes("IfcSIUnit"));
    assert.deepEqual(select("IfcSizeSelect"), schema === "IFC2X3"
        ? ["IfcRatioMeasure", "IfcLengthMeasure", "IfcDescriptiveMeasure", "IfcPositiveLengthMeasure", "IfcNormalisedRatioMeasure", "IfcPositiveRatioMeasure"]
        : ["IfcDescriptiveMeasure", "IfcLengthMeasure", "IfcNormalisedRatioMeasure", "IfcPositiveLengthMeasure", "IfcPositiveRatioMeasure", "IfcRatioMeasure"]);
    assert.deepEqual(select("IfcRotationalStiffnessSelect"), schema === "IFC2X3"
        ? [] : ["IfcBoolean", "IfcRotationalStiffnessMeasure"]);
    assert.deepEqual(select("IfcSegmentIndexSelect"), schema === "IFC2X3" ? [] : ["IfcArcIndex", "IfcLineIndex"]);
    if (schema !== "IFC2X3") {
        assert.deepEqual(named("IfcArcIndex"), {baseType: "INTEGER", aggregates: ["LIST"]});
        assert.deepEqual(named("IfcLineIndex"), {baseType: "INTEGER", aggregates: ["LIST"]});
    }
    const values = select("IfcValue");
    assert.ok(values.includes("IfcPositiveLengthMeasure"));
    assert.ok(values.includes("IfcLengthMeasure"));
    assert.ok(values.includes("IfcComplexNumber"));
    assert.ok(values.every(name => types.some(type => type.name === name)));
    assert.deepEqual(select("IfcAppliedValueSelect"), schema === "IFC2X3"
        ? ["IfcRatioMeasure", "IfcMeasureWithUnit", "IfcMonetaryMeasure"]
        : ["IfcMeasureWithUnit", "IfcReference", ...values]);
    assert.deepEqual(select("IfcDoesNotExist"), []);
    assert.deepEqual(select("IfcLabel"), []);
    assert.deepEqual(select("IfcWall"), []);
    assert.deepEqual(select("IfcPropertySetDefinitionSelect"), schema === "IFC2X3"
        ? [] : ["IfcPropertySetDefinition", "IfcPropertySetDefinitionSet"]);
    const trim = entities.find(entity => entity.name === "IfcTrimmedCurve")!.props.find(prop => prop.name === "Trim1")!;
    assert.deepEqual(trim.expressType, {typeName: "IfcTrimmingSelect", aggregates: ["SET"]});
    assert.deepEqual(select(trim.expressType.typeName), ["IfcCartesianPoint", "IfcParameterValue"]);
    assert.deepEqual(resolveType(trim.expressType), {baseType: "SELECT", aggregates: ["SET"], resolvedTypeName: "IfcTrimmingSelect"});
    const exponent = entities.find(entity => entity.name === "IfcDimensionalExponents")!.props[0];
    assert.equal(exponent.expressType.typeName, "INTEGER");
    assert.equal(resolveType(exponent.expressType).baseType, "INTEGER");
    if (schema !== "IFC2X3") {
        const controlPoints = entities.find(entity => entity.name === "IfcBSplineSurface")!.props.find(prop => prop.name === "ControlPointsList")!;
        assert.deepEqual(resolveType(controlPoints.expressType), {baseType: "ENTITY", aggregates: ["LIST", "LIST"], resolvedTypeName: "IfcCartesianPoint"});
        assert.deepEqual(named("IfcPropertySetDefinitionSet"), {baseType: "ENTITY", aggregates: ["SET"], resolvedTypeName: "IfcPropertySetDefinition"});
        const namedTypes = header.split(`kNamedTypes_${schema} = {{`)[1].split("}};")[0];
        assert.ok(namedTypes.includes(`{${crc32("IFCPROPERTYSETDEFINITIONSET", crcTable)}, {ExpressBaseType::ENTITY, kAggregates_${schema}_SET, ${crc32("IFCPROPERTYSETDEFINITION", crcTable)}}}`));
        assert.ok(namedTypes.includes(`{${crc32("IFCVALUE", crcTable)}, {ExpressBaseType::SELECT, std::span<const AggregateKind>{}, ${crc32("IFCVALUE", crcTable)}}}`));
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
        const entries = array.match(/\{ExpressBaseType::\w+, (?:std::span<const AggregateKind>\{\}|kAggregates_\w+)(?:, \d+)?\}/g)!;
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
const selectFixture = parseElements("ENTITY Item; END_ENTITY; TYPE Named = REAL; END_TYPE; TYPE Group = ARRAY [1:2] OF Named; END_TYPE; TYPE Inner = SELECT (Named, Item); END_TYPE; TYPE Outer = SELECT (Inner, Named, Group, Inner); END_TYPE; TYPE Missing = SELECT (DoesNotExist); END_TYPE; TYPE CycleA = SELECT (CycleB); END_TYPE; TYPE CycleB = SELECT (CycleA); END_TYPE; TYPE SelfCycle = SELECT (SelfCycle); END_TYPE; TYPE Empty = SELECT (); END_TYPE;");
assert.deepEqual(resolveSelectCandidates("Outer", selectFixture.types, selectFixture.entities), ["Named", "Item", "Group"]);
assert.throws(() => resolveSelectCandidates("Missing", selectFixture.types, selectFixture.entities), /Unresolved SELECT candidate: DoesNotExist/);
assert.throws(() => resolveSelectCandidates("CycleA", selectFixture.types, selectFixture.entities), /Cyclic SELECT candidate: CycleA/);
assert.throws(() => resolveSelectCandidates("SelfCycle", selectFixture.types, selectFixture.entities), /Cyclic SELECT candidate: SelfCycle/);
assert.throws(() => resolveSelectCandidates("Empty", selectFixture.types, selectFixture.entities), /Unresolved SELECT candidate/);
const identityFixture = parseElements("ENTITY Item; END_ENTITY; TYPE EntityAlias = Item; END_TYPE; TYPE EntityGroup = ARRAY [1:2] OF EntityAlias; END_TYPE; TYPE Choice = SELECT (Item); END_TYPE; TYPE SelectAlias = Choice; END_TYPE; TYPE SelectGroup = LIST [1:?] OF SelectAlias; END_TYPE; TYPE CycleA = CycleB; END_TYPE; TYPE CycleB = CycleA; END_TYPE;");
assert.deepEqual(resolveExpressType(parseExpressType("SET [1:?] OF EntityGroup"), identityFixture.types, identityFixture.entities), {baseType: "ENTITY", aggregates: ["SET", "ARRAY"], resolvedTypeName: "Item"});
assert.deepEqual(resolveExpressType(parseExpressType("SelectAlias"), identityFixture.types, identityFixture.entities), {baseType: "SELECT", aggregates: [], resolvedTypeName: "Choice"});
assert.deepEqual(resolveExpressType(parseExpressType("ARRAY [1:2] OF SelectGroup"), identityFixture.types, identityFixture.entities), {baseType: "SELECT", aggregates: ["ARRAY", "LIST"], resolvedTypeName: "Choice"});
for (const name of ["CycleA", "DoesNotExist", "REAL"]) {
    const resolved = resolveExpressType(parseExpressType(name), identityFixture.types, identityFixture.entities);
    assert.equal(resolved.resolvedTypeName ? crc32(resolved.resolvedTypeName.toUpperCase(), crcTable) : 0, 0);
}
console.log("EXPRESS type metadata checks passed for all three schemas.");
