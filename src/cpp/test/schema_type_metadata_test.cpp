#include "TinyCppTest.hpp"
#include "../web-ifc/schema/schema-type-metadata.h"
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
}
