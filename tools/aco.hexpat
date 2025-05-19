// partial hexpat reader -- used to parse aco files, but not completely
// don't expect a lot

#pragma endian big

struct Header {
  u16 version;
  u16 numColors;
};

struct ColorSpecV1 {
    u16 colorSpace;
    u16 w, x, y, z;
};

struct ColorSpecV2 {
    u16 zero;
    u16 lenPlusOne;
    
};

struct ACO {
    Header headerV1;
    ColorSpecV1 spec[headerV1.numColors];
    
    Header headerV2;
    //ColorSpecV2 spec2;
};

ACO aco @ 0x00;