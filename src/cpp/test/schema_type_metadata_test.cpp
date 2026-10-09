#include "TinyCppTest.hpp"
#include "../web-ifc/schema/schema-type-metadata.h"
#include "../web-ifc/schema/schema-inverses.h"
#include <algorithm>
#include <limits>

using namespace webifc::schema;

TEST(SchemaTypeMetadata)
{
    for (auto schema : {IFC2X3, IFC4, IFC4X3}) {
        ASSERT(getTypeMetadata(schema, IFCPOSITIVELENGTHMEASURE).base_type == ExpressBaseType::REAL);
        ASSERT(getTypeMetadata(schema, IFCINTEGER).base_type == ExpressBaseType::INTEGER);
        ASSERT(getTypeMetadata(schema, IFCCOUNTMEASURE).base_type ==
               (schema == IFC4X3 ? ExpressBaseType::INTEGER : ExpressBaseType::NUMBER));
        ASSERT(getTypeMetadata(schema, IFCLABEL).base_type == ExpressBaseType::STRING);
        ASSERT(getTypeMetadata(schema, IFCBOOLEAN).base_type == ExpressBaseType::BOOLEAN);
        ASSERT(getTypeMetadata(schema, IFCLOGICAL).base_type == ExpressBaseType::LOGICAL);
        ASSERT(getTypeMetadata(schema, IFCUNITENUM).base_type == ExpressBaseType::ENUM);
        ASSERT(getTypeMetadata(schema, IFCVALUE).base_type == ExpressBaseType::SELECT);
        ASSERT(getTypeMetadata(schema, IFCWALL).base_type == ExpressBaseType::ENTITY);
        ASSERT(getTypeMetadata(schema, IFCGEOMETRICREPRESENTATIONITEM).base_type == ExpressBaseType::ENTITY);
        ASSERT(getTypeMetadata(schema, IFCLABEL).aggregates.empty());
        ASSERT(getTypeMetadata(schema, IFCWALL).resolved_type_code == IFCWALL);
        ASSERT(getTypeMetadata(schema, IFCVALUE).resolved_type_code == IFCVALUE);
        ASSERT(getTypeMetadata(schema, IFCLABEL).resolved_type_code == 0);
        ASSERT(getPropertyTypeMetadata(schema, IFCWALL, 5).resolved_type_code == IFCOBJECTPLACEMENT);
        ASSERT(getPropertyTypeMetadata(schema, IFCSIUNIT, 0).resolved_type_code == IFCDIMENSIONALEXPONENTS);
        ASSERT(getPropertyTypeMetadata(schema, IFCWALL, 0).resolved_type_code == 0);

        auto complex = getTypeMetadata(schema, IFCCOMPLEXNUMBER);
        ASSERT(complex.base_type == ExpressBaseType::REAL);
        ASSERT(complex.aggregates.size() == 1 && complex.aggregates[0] == AggregateKind::ARRAY);
        auto angle = getTypeMetadata(schema, IFCCOMPOUNDPLANEANGLEMEASURE);
        ASSERT(angle.base_type == ExpressBaseType::INTEGER);
        ASSERT(angle.aggregates.size() == 1 && angle.aggregates[0] == AggregateKind::LIST);

        auto coordinates = getPropertyTypeMetadata(schema, IFCCARTESIANPOINT, 0);
        ASSERT(coordinates.base_type == ExpressBaseType::REAL);
        ASSERT(coordinates.aggregates.size() == 1 && coordinates.aggregates[0] == AggregateKind::LIST);
        ASSERT(getPropertyTypeMetadata(schema, IFCQUANTITYLENGTH, 3).base_type == ExpressBaseType::REAL);
        ASSERT(getPropertyTypeMetadata(schema, IFCDIMENSIONALEXPONENTS, 0).base_type == ExpressBaseType::INTEGER);
        ASSERT(getPropertyTypeMetadata(schema, IFCBSPLINECURVE, 3).base_type == ExpressBaseType::LOGICAL);
        ASSERT(getPropertyTypeMetadata(schema, IFCCOMPOSITECURVESEGMENT, 1).base_type == ExpressBaseType::BOOLEAN);
        ASSERT(getPropertyTypeMetadata(schema, IFCWALL, 0).base_type == ExpressBaseType::STRING);
        ASSERT(getPropertyTypeMetadata(schema, IFCWALL, 5).base_type == ExpressBaseType::ENTITY);
        // Dimensions remains position 0 even though IfcSIUnit redeclares it as DERIVE.
        ASSERT(getPropertyTypeMetadata(schema, IFCSIUNIT, 0).base_type == ExpressBaseType::ENTITY);
        ASSERT(getPropertyTypeMetadata(schema, IFCSIUNIT, 1).base_type == ExpressBaseType::ENUM);

        ASSERT(getTypeMetadata(schema, 0).base_type == ExpressBaseType::UNKNOWN);
        auto unknown = getPropertyTypeMetadata(schema, IFCWALL, std::numeric_limits<uint32_t>::max());
        ASSERT(unknown.base_type == ExpressBaseType::UNKNOWN);
        ASSERT(unknown.aggregates.empty());
        ASSERT(unknown.resolved_type_code == 0);
        ASSERT(getTypeMetadata(schema, 0).resolved_type_code == 0);
        ASSERT(getPropertyTypeMetadata(schema, IFCGEOMETRICREPRESENTATIONITEM, 0).base_type == ExpressBaseType::UNKNOWN);
        ASSERT(getPropertyTypeMetadata(schema, IFCLABEL, 0).base_type == ExpressBaseType::UNKNOWN);
        ASSERT(getPropertyTypeMetadata(schema, 0, 0).base_type == ExpressBaseType::UNKNOWN);
    }
    for (auto schema : {IFC4, IFC4X3}) {
        ASSERT(getTypeMetadata(schema, IFCBINARY).base_type == ExpressBaseType::BINARY);
        auto nested = getPropertyTypeMetadata(schema, IFCCARTESIANPOINTLIST3D, 0);
        ASSERT(nested.base_type == ExpressBaseType::REAL);
        ASSERT(nested.aggregates.size() == 2 && nested.aggregates[0] == AggregateKind::LIST &&
               nested.aggregates[1] == AggregateKind::LIST);
    }
    ASSERT(getTypeMetadata(IFC2X3, IFCBINARY).base_type == ExpressBaseType::UNKNOWN);
    ASSERT(getTypeMetadata(IFC4X3, IFCDOORSTYLE).base_type == ExpressBaseType::UNKNOWN);
    ASSERT(getPropertyTypeMetadata(IFC4X3, IFCDOORSTYLE, 0).base_type == ExpressBaseType::UNKNOWN);
    auto invalidSchema = static_cast<IFC_SCHEMA>(3);
    ASSERT(getTypeMetadata(invalidSchema, IFCWALL).base_type == ExpressBaseType::UNKNOWN);
    ASSERT(getPropertyTypeMetadata(invalidSchema, IFCWALL, 0).base_type == ExpressBaseType::UNKNOWN);
    ASSERT(getTypeMetadata(invalidSchema, IFCWALL).resolved_type_code == 0);
    ExpressTypeMetadata primitive{ExpressBaseType::REAL, {}};
    ASSERT(primitive.resolved_type_code == 0);
}

TEST(SchemaSelectCandidates)
{
    for (auto schema : {IFC2X3, IFC4, IFC4X3}) {
        auto actors = getSelectCandidates(schema, IFCACTORSELECT);
        ASSERT(actors.size() == 3);
        ASSERT(actors[0] == IFCORGANIZATION && actors[1] == IFCPERSON &&
               actors[2] == IFCPERSONANDORGANIZATION);
        auto trimming = getSelectCandidates(schema, IFCTRIMMINGSELECT);
        ASSERT(trimming.size() == 2 && trimming[0] == IFCCARTESIANPOINT &&
               trimming[1] == IFCPARAMETERVALUE);
        auto trimAttribute = getPropertyTypeMetadata(schema, IFCTRIMMEDCURVE, 1);
        ASSERT(trimAttribute.base_type == ExpressBaseType::SELECT);
        ASSERT(trimAttribute.aggregates.size() == 1 && trimAttribute.aggregates[0] == AggregateKind::SET);
        ASSERT(trimAttribute.resolved_type_code == IFCTRIMMINGSELECT);
        ASSERT(getSelectCandidates(schema, trimAttribute.resolved_type_code).data() == trimming.data());

        auto values = getSelectCandidates(schema, IFCVALUE);
        auto sizes = getSelectCandidates(schema, IFCSIZESELECT);
        ASSERT(sizes.size() == 6);
        ASSERT(std::find(sizes.begin(), sizes.end(), IFCPOSITIVELENGTHMEASURE) != sizes.end());
        ASSERT(std::find(sizes.begin(), sizes.end(), IFCLENGTHMEASURE) != sizes.end());
        ASSERT(!values.empty());
        ASSERT(std::find(values.begin(), values.end(), IFCPOSITIVELENGTHMEASURE) != values.end());
        ASSERT(std::find(values.begin(), values.end(), IFCLENGTHMEASURE) != values.end());
        ASSERT(std::find(values.begin(), values.end(), IFCCOMPLEXNUMBER) != values.end());
        for (auto candidate : values) {
            auto base = getTypeMetadata(schema, candidate).base_type;
            ASSERT(base != ExpressBaseType::SELECT && base != ExpressBaseType::ENTITY &&
                   base != ExpressBaseType::UNKNOWN);
            ASSERT(std::count(values.begin(), values.end(), candidate) == 1);
        }
        auto applied = getSelectCandidates(schema, IFCAPPLIEDVALUESELECT);
        if (schema == IFC2X3) {
            ASSERT(applied.size() == 3 && applied[0] == IFCRATIOMEASURE &&
                   applied[1] == IFCMEASUREWITHUNIT && applied[2] == IFCMONETARYMEASURE);
        } else {
            ASSERT(applied.size() == values.size() + 2);
            ASSERT(applied[0] == IFCMEASUREWITHUNIT && applied[1] == IFCREFERENCE);
            ASSERT(std::equal(values.begin(), values.end(), applied.begin() + 2));
        }
        auto units = getSelectCandidates(schema, IFCUNIT);
        ASSERT(units.size() == 3 && units[0] == IFCDERIVEDUNIT);
        ASSERT(units[1] == (schema == IFC2X3 ? IFCNAMEDUNIT : IFCMONETARYUNIT));
        ASSERT(units[2] == (schema == IFC2X3 ? IFCMONETARYUNIT : IFCNAMEDUNIT));
        ASSERT(std::find(units.begin(), units.end(), IFCSIUNIT) == units.end());
        auto subtypes = getInheritedTypes(schema, IFCNAMEDUNIT);
        ASSERT(std::find(subtypes.begin(), subtypes.end(), IFCSIUNIT) != subtypes.end());

        ASSERT(getSelectCandidates(schema, 0).empty());
        ASSERT(getSelectCandidates(schema, IFCLABEL).empty());
        ASSERT(getSelectCandidates(schema, IFCWALL).empty());
        ASSERT(getSelectCandidates(schema, IFCVALUE).data() == values.data());
    }
    for (auto schema : {IFC4, IFC4X3}) {
        auto stiffness = getSelectCandidates(schema, IFCROTATIONALSTIFFNESSSELECT);
        ASSERT(stiffness.size() == 2 && stiffness[0] == IFCBOOLEAN &&
               stiffness[1] == IFCROTATIONALSTIFFNESSMEASURE);
        auto indices = getSelectCandidates(schema, IFCSEGMENTINDEXSELECT);
        ASSERT(indices.size() == 2 && indices[0] == IFCARCINDEX && indices[1] == IFCLINEINDEX);
        auto arc = getTypeMetadata(schema, indices[0]);
        auto line = getTypeMetadata(schema, indices[1]);
        ASSERT(arc.base_type == ExpressBaseType::INTEGER && arc.aggregates.size() == 1 &&
               arc.aggregates[0] == AggregateKind::LIST);
        ASSERT(line.base_type == ExpressBaseType::INTEGER && line.aggregates.size() == 1 &&
               line.aggregates[0] == AggregateKind::LIST);
        auto definitions = getSelectCandidates(schema, IFCPROPERTYSETDEFINITIONSELECT);
        ASSERT(definitions.size() == 2 && definitions[0] == IFCPROPERTYSETDEFINITION &&
               definitions[1] == IFCPROPERTYSETDEFINITIONSET);
        auto aggregate = getTypeMetadata(schema, definitions[1]);
        ASSERT(aggregate.base_type == ExpressBaseType::ENTITY);
        ASSERT(aggregate.aggregates.size() == 1 && aggregate.aggregates[0] == AggregateKind::SET);
        ASSERT(aggregate.resolved_type_code == IFCPROPERTYSETDEFINITION);
        aggregate.aggregates = aggregate.aggregates.subspan(1);
        ASSERT(aggregate.aggregates.empty());
        ASSERT(aggregate.resolved_type_code == IFCPROPERTYSETDEFINITION);
        auto definitionsSubtypes = getInheritedTypes(schema, aggregate.resolved_type_code);
        ASSERT(std::find(definitionsSubtypes.begin(), definitionsSubtypes.end(), IFCPROPERTYSET) != definitionsSubtypes.end());
        ASSERT(std::find(definitionsSubtypes.begin(), definitionsSubtypes.end(), IFCELEMENTQUANTITY) != definitionsSubtypes.end());
        ASSERT(std::find(definitionsSubtypes.begin(), definitionsSubtypes.end(), IFCWALL) == definitionsSubtypes.end());
        auto definitionAttribute = getPropertyTypeMetadata(schema, IFCRELDEFINESBYPROPERTIES, 5);
        ASSERT(definitionAttribute.resolved_type_code == IFCPROPERTYSETDEFINITIONSELECT);
        auto controlPoints = getPropertyTypeMetadata(schema, IFCBSPLINESURFACE, 2);
        ASSERT(controlPoints.resolved_type_code == IFCCARTESIANPOINT);
        ASSERT(controlPoints.aggregates.size() == 2);
        controlPoints.aggregates = controlPoints.aggregates.subspan(1);
        ASSERT(controlPoints.resolved_type_code == IFCCARTESIANPOINT);
    }
    ASSERT(getSelectCandidates(IFC2X3, IFCPROPERTYSETDEFINITIONSELECT).empty());
    ASSERT(getSelectCandidates(IFC2X3, IFCROTATIONALSTIFFNESSSELECT).empty());
    ASSERT(getSelectCandidates(IFC2X3, IFCSEGMENTINDEXSELECT).empty());
    ASSERT(getSelectCandidates(static_cast<IFC_SCHEMA>(3), IFCVALUE).empty());
}
