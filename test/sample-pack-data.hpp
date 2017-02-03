#ifndef SAMPLE_PACK_DATA
#define SAMPLE_PACK_DATA

namespace data {

struct expected_objects {
    std::string name;
    std::string type;
    size_t size;
    size_t pack_size;
    size_t offset;
    size_t depth;
    std::string parent;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

inline const std::vector<expected_objects>& get_expected_objects() {
    static const std::vector<expected_objects> data {
        { "31b3469089c90a7d1e1177a38a07e6be9b0c4e6f", "commit",  76,  88,  449, 1,  "fc4f86dd288c1286cf2fd5c8f34cc1e43351c79b" },
        { "3bb2a5be07fc75b1edfecd7ade1b29261850526e", "commit", 258, 175,  537 },
        { "4b825dc642cb6eb9a060e54bf8d69288fbee4904", "tree",    0,   9, 1086 },
        { "5fab85dd8b213db1ff45e64513f45ebde6edb53a", "commit",  81,  93,  183, 1,  "7dec0ebb72f5558c2f107f7833dbd90419c1710e" },
        { "78981922613b2afb6025042ff6bd878ac1994e85", "blob",    2,  11,  912 },
        { "7dec0ebb72f5558c2f107f7833dbd90419c1710e", "commit", 258, 171,   12 },
        { "82498d0e24a63d42870c56b0376bb35f59118c80", "commit",  33,  45,  712, 1,  "3bb2a5be07fc75b1edfecd7ade1b29261850526e" },
        { "9102c76e9c6f2b5bcbfd71c27958f800194b3f68", "commit", 176, 119,  793 },
        { "aaff74984cccd156a469afa7d9ab10e4777beb24", "tree",   29,  39, 1047 },
        { "bfdaa0f1c3415c09d3080063911d155fd7259d18", "blob",    5,  14,  923 },
        { "c7b1cff039a93f3600a1d18b82d26688668c7dea", "tree",   58,  45, 1002 },
        { "decdd2877670620312624ce55de005f4517b4c5b", "commit",  25,  36,  757, 2,  "82498d0e24a63d42870c56b0376bb35f59118c80" },
        { "e9f67bff01c541b6b8c7259450260a85e27aa006", "tree",   58,  65,  937 },
        { "fc4f86dd288c1286cf2fd5c8f34cc1e43351c79b", "commit", 258, 173,  276 }
    };

    return data;
}

#pragma GCC diagnostic pop

}

#endif
