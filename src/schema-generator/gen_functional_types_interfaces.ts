
export type ExpressBaseType = "INTEGER" | "REAL" | "NUMBER" | "STRING" | "BOOLEAN" | "LOGICAL" | "BINARY" | "ENUM" | "SELECT" | "ENTITY" | "UNKNOWN";
export type AggregateKind = "LIST" | "SET" | "ARRAY" | "BAG";

export interface ExpressType {
    typeName: string;
    aggregates: AggregateKind[];
}

export interface Type {
    name: string;
    typeName : string;
    typeNum: number;
    isList: boolean;
    isEnum: boolean;
    isSelect: boolean;
    values: string[];
    expressType: ExpressType;
}

export interface Prop {
    name: string;
    type: string;
    typeNum: number;
    primitive: boolean;
    optional: boolean;
    set: boolean;
    dimensions: number;
    expressType: ExpressType;
}

export interface InverseProp {
    name: string;
    type: string;
    set: boolean;
    for: string;
}

export interface Entity {
    name: string;
    parent: null | string;
    children: string[];
    props: Prop[];
    inverseProps: InverseProp[],
    derivedProps: Prop[];
    ifcDerivedProps: string[];
    derivedInverseProps: InverseProp[],
    isIfcProduct: boolean;
}

export interface Param
{
    name: string;
    type: string;
    prop: Prop;
    isType: Type;
}