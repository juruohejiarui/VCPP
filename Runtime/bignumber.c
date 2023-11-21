#include "vmobj.h" 

#define DIGTAL_SIZE 10000
int64_t TempData[200005], TempDataLength;

struct BigNumber *VM_BigNumber_Create(int64_t _init_val) {
    struct BigNumber *_object = VM_CreateBuiltinObject(0, ObjectType_BigNumber);
    if (_init_val < 0) _object->Sign = -1, _init_val = -_init_val;
    else _object->Sign = 1;
    if (_init_val == 0) {
        _object->Length = 1;
        _object->Data = malloc(sizeof(uint16_t)), _object->Data[0] = 0;
    } else {
        TempDataLength = 0;
        while (_init_val) 
            TempData[TempDataLength++] = _init_val % DIGTAL_SIZE,
            _init_val /= DIGTAL_SIZE;
        _object->Length = TempDataLength;
        _object->Data = malloc(sizeof(uint16_t) * TempDataLength);
        for (int i = 0; i < _object->Length; i++) _object->Data[i] = TempData[i];
    }
    return _object;
}

void VM_BigNumber_Free(struct BigNumber * _object) {
    free(_object->Data);
}
void VM_BigNumber_Assign(struct BigNumber *_object, struct BigNumber *_a) {
    _object->Length = _a->Length, _object->Sign = _a->Sign;
    free(_object->Data);
    _object->Data = malloc(sizeof(uint16_t) * _object->Length);
    memcpy(_object->Data, _a->Data, sizeof(uint16_t) * _object->Length);
}

// create a big number object using TempData
struct BigNumber *CreateBigNumberObject() {
    struct BigNumber *_res = VM_CreateBuiltinObject(0, ObjectType_BigNumber);
    _res->Length = TempDataLength;
    _res->Data = malloc(sizeof(uint16_t) * _res->Length);
    for (int i = 0; i < _res->Length; i++) _res->Data[i] = TempData[i];
    return _res;
}

// Calculate |_n1| + |_n2|
struct BigNumber *AbsoluateAdd(struct BigNumber *_n1, struct BigNumber *_n2) {
    TempDataLength = max(_n1->Length, _n2->Length);
    for (int i = 0; i < min(_n1->Length, _n2->Length); i++) 
        TempData[i] = (int64_t)_n1->Data[i] + (int64_t)_n2->Data[i];
    for (int i = min(_n1->Length, _n2->Length); i < _n1->Length; i++) TempData[i] = (int64_t)_n1->Data[i];
    for (int i = min(_n1->Length, _n2->Length); i < _n2->Length; i++) TempData[i] = (int64_t)_n2->Data[i];

    for (int i = 0; i < TempDataLength; i++) 
        TempData[i + 1] += TempData[i] % DIGTAL_SIZE, TempData[i] /= DIGTAL_SIZE;
    while (TempData[TempDataLength]) TempDataLength++;

    return CreateBigNumberObject();
}
// Calculate |_n1| - |_n2|, you must guarantee that |_n1| >= |_n2|
struct BigNumber *AbsoluateMinus(struct BigNumber *_n1, struct BigNumber *_n2) {
    TempDataLength = _n1->Length;
    for (int i = 0; i < _n1->Length; i++) TempData[i] = (int64_t)_n1->Data[i];
    for (int i = 0; i < _n1->Length; i++) {
        if (i < _n2->Length && _n2->Data[i] > TempData[i]) {
            TempData[i + 1]--;
            TempData[i] += DIGTAL_SIZE;
        }
        TempData[i] -= _n2->Data[i];
    }
    while (!TempData[TempDataLength] && TempDataLength > 1) TempDataLength--;

    return CreateBigNumberObject();
}
// compare |_n1| and |_n2|
int AbsoluateCompare(struct BigNumber *_n1, struct BigNumber *_n2) {
    if (_n1->Length != _n2->Length) return (_n1->Length > _n2->Length ? 1 : -1);
    for (int i = _n1->Length - 1; i >= 0; i--) if (_n1->Data[i] != _n2->Data[i]) 
        return _n1->Data[i] > _n2->Data[i] ? 1 : -1;
    return 0;
}
struct BigNumber *VM_BigNumber_Add(struct BigNumber *_n1, struct BigNumber *_n2) {
    struct BigNumber *_res = NULL;
    if (_n1->Sign == _n2->Sign) _res = AbsoluateAdd(_n1, _n2), _res->Sign = _n1->Sign;
    else {
        int _cres = AbsoluateCompare(_n1, _n2);
        // (_n1 > -_n2 and _n2 < 0) or (-_n1 > _n2 and _n1 < 0)
        if ((_n2->Sign < 0 && _cres >= 0) || (_n1->Sign < 0 && _cres < 0))
            _res = AbsoluateMinus(_n1, _n2), _res->Sign = _n1->Sign;
        else 
            _res = AbsoluateMinus(_n2, _n1), _res->Sign = _n2->Sign;
    }
    return _res;
}

struct BigNumber *VM_BigNumber_Minus(struct BigNumber *_n1, struct BigNumber *_n2) {
    struct BigNumber *_res = NULL;
    if (_n1->Sign != _n2->Sign) _res = AbsoluateAdd(_n1, _n2), _res->Sign = _n1->Sign;
    else {
        int _cres = AbsoluateCompare(_n1, _n2);
        if (_cres >= 0) _res = AbsoluateMinus(_n1, _n2), _res->Sign = _n1->Sign;
        else _res = AbsoluateMinus(_n2, _n1), _res->Sign = -_n1->Sign;
    }
    return _res;
}

struct BigNumber *VM_BigNumber_Mul(struct BigNumber *_n1, struct BigNumber *_n2) {
    TempDataLength = _n1->Length + _n2->Length;
    memset(TempData, 0, sizeof(int64_t) * TempDataLength);
    // using the most simple one, I will update it using FFT algorithm once I have more spare time 
    for (int i = 0; i < _n1->Length; i++)
        for (int j = 0; j < _n2->Length; j++)
            TempData[i + j] += _n1->Data[i] * _n2->Data[j];
    for (int i = 0; i < TempDataLength; i++)
        TempData[i + 1] += TempData[i] / 10, TempData[i] %= 10;
    while (!TempData[TempDataLength - 1] && TempDataLength > 1) TempDataLength--;

    struct BigNumber *_res = CreateBigNumberObject();
    _res->Sign = (_n1->Sign == _n2->Sign ? 1 : -1);
    return _res;
}

int SequenceCompare(uint64_t *_a, uint64_t *_b, int _len) {
    for (int i = 0; i < _len; i++) if (_a[i] != _b[i]) return _a[i] > _b[i] ? 1 : -1;
    return 0;
}
// _n1 -= _n2
void Sub(uint64_t *_n1, uint64_t *_n2, uint64_t _len) {
    for (int i = _len - 1; i >= 0; i--) {
        if (_n1[i] < _n2[i]) _n1[i + 1]--, _n1[i] += DIGTAL_SIZE;
        _n1[i] -= _n2[i];
    }
}

struct BigNumber *VM_BigNumber_Div(struct BigNumber *_n1, struct BigNumber *_n2) {
    static uint64_t _temp_data[10][200005], _temp_data_length[10];
    if (AbsoluateCompare(_n1, _n2) < 0) return VM_BigNumber_Create(0);
    TempDataLength = _n1->Length - _n2->Length + 1;
    for (int i = 0; i < _n1->Length; i++) TempData[i] = _n1->Data[i];

    // calculate _n2 * 1, _n2 * 2, _n2 * 3 ... _n2 * 9
    for (int v = 1; v <= 9; v++) {
        _temp_data_length[v] = _n2->Length + 1, _temp_data[v][_n2->Length] = 0;
        for (int i = 0; i < _n2->Length; i++) 
            _temp_data[v][i] = _n2->Data[i] * v;
        for (int i = 0; i < _n2->Length; i++) 
            _temp_data[v][i + 1] += _temp_data[v][i] / DIGTAL_SIZE, _temp_data[v][i] /= DIGTAL_SIZE;
        if (_temp_data[_n2->Length] > 0) _temp_data_length[v]++;
    }
    for (int i = TempDataLength - 1; i >= 0; i--) {
        int l = 1, r = 9;
        while (l <= r) {
            int mid = (l + r) >> 1;
            if (_temp_data_length[mid] <= TempDataLength - i
                 && SequenceCompare(TempData + i, _temp_data[mid], _temp_data_length[mid]))
                l = mid + 1;
            else r = mid - 1;
            TempData[i] = l - 1;
            Sub(TempData + i, _temp_data[l - 1], _temp_data_length[l - 1]);
        }
    }
    while (TempDataLength > 1 && !TempData[TempDataLength - 1]) TempDataLength--;

    struct BigNumber *_res = CreateBigNumberObject();
    _res->Sign = (_n1->Sign == _n2->Sign ? 1 : -1);
    return _res;
}


