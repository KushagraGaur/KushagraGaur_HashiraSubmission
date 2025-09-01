#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <tuple>
#include <cstdint>
#include <utility>

#include "json.hpp" 

using json = nlohmann::json;

struct Big {
    static const uint32_t BASE = 1000000000u;
    std::vector<uint32_t> d; 

    Big() {}
    explicit Big(uint64_t v) { while (v) { d.push_back(uint32_t(v % BASE)); v /= BASE; } }

    bool isZero() const { return d.empty(); }
    void trim() { while (!d.empty() && d.back()==0) d.pop_back(); }

    static int cmp(const Big& a, const Big& b){
        if (a.d.size()!=b.d.size()) return a.d.size()<b.d.size()? -1:1;
        for (int i=(int)a.d.size()-1;i>=0;--i)
            if (a.d[i]!=b.d[i]) return a.d[i]<b.d[i]? -1:1;
        return 0;
    }

    static Big add(const Big& a, const Big& b){
        Big c; c.d.resize(std::max(a.d.size(), b.d.size())+1, 0);
        uint64_t carry=0; size_t i=0;
        for (; i<std::max(a.d.size(), b.d.size()); ++i){
            uint64_t s = carry;
            if (i<a.d.size()) s += a.d[i];
            if (i<b.d.size()) s += b.d[i];
            c.d[i] = uint32_t(s % BASE);
            carry = s / BASE;
        }
        if (carry) c.d[i++] = (uint32_t)carry;
        c.d.resize(i);
        c.trim();
        return c;
    }

    // assumes a >= b
    static Big sub(const Big& a, const Big& b){
        Big c; c.d.resize(a.d.size(), 0);
        int64_t carry=0;
        for (size_t i=0;i<a.d.size();++i){
            int64_t cur = (int64_t)a.d[i] - (i<b.d.size()?b.d[i]:0) + carry;
            if (cur < 0) { cur += BASE; carry = -1; } else carry = 0;
            c.d[i] = (uint32_t)cur;
        }
        c.trim();
        return c;
    }

    static Big mul_small(const Big& a, uint32_t m){
        if (a.isZero() || m==0) return Big();
        Big c; c.d.resize(a.d.size()+1, 0);
        uint64_t carry=0; size_t i=0;
        for (; i<a.d.size(); ++i){
            uint64_t cur = (uint64_t)a.d[i]*m + carry;
            c.d[i] = (uint32_t)(cur % BASE);
            carry = cur / BASE;
        }
        if (carry) c.d[i++] = (uint32_t)carry;
        c.d.resize(i);
        c.trim();
        return c;
    }

    static Big mul(const Big& a, const Big& b){
        if (a.isZero() || b.isZero()) return Big();
        Big c; c.d.assign(a.d.size()+b.d.size(), 0);
        for (size_t i=0;i<a.d.size();++i){
            uint64_t carry=0;
            for (size_t j=0;j<b.d.size() || carry; ++j){
                unsigned __int128 cur = c.d[i+j];
                if (j<b.d.size()) cur += (unsigned __int128)a.d[i]*(unsigned __int128)b.d[j];
                cur += carry;
                c.d[i+j] = (uint32_t)((uint64_t)cur % BASE);
                carry = (uint64_t)(cur / BASE);
            }
        }
        c.trim();
        return c;
    }

    std::string toString() const {
        if (d.empty()) return "0";
        std::ostringstream oss;
        oss << d.back();
        for (int i=(int)d.size()-2;i>=0;--i){
            oss << std::setw(9) << std::setfill('0') << d[i];
        }
        return oss.str();
    }
};

struct SBig {
    Big m; bool neg=false;
    SBig() {}
    explicit SBig(long long v){ if (v<0){ neg=true; m=Big((uint64_t)(-v)); } else m=Big((uint64_t)v); norm(); }
    void norm(){ if (m.isZero()) neg=false; }
    static SBig from(const Big& b, bool n){ SBig s; s.m=b; s.neg=n; s.norm(); return s; }

    static SBig add(const SBig& a, const SBig& b){
        if (a.neg==b.neg){ SBig r; r.m=Big::add(a.m,b.m); r.neg=a.neg; r.norm(); return r; }
        int c=Big::cmp(a.m,b.m);
        if (c==0) return SBig(0);
        if (c>0){ SBig r; r.m=Big::sub(a.m,b.m); r.neg=a.neg; r.norm(); return r; }
        else    { SBig r; r.m=Big::sub(b.m,a.m); r.neg=b.neg; r.norm(); return r; }
    }
    static SBig sub(const SBig& a, const SBig& b){ SBig nb=b; nb.neg=!nb.neg; return add(a,nb); }
    static SBig mul(const SBig& a, const SBig& b){ SBig r; r.m=Big::mul(a.m,b.m); r.neg=(a.neg!=b.neg); r.norm(); return r; }
    std::string toString() const { return m.isZero()? "0" : (neg?("-"+m.toString()):m.toString()); }
};

//base conversion & poly multiply
static inline int digit_of(char c){
    if (c>='0' && c<='9') return c-'0';
    if (c>='a' && c<='f') return 10+(c-'a');
    if (c>='A' && c<='F') return 10+(c-'A');
    return -1;
}

static Big base_to_big(const std::string& s, int base){
    Big res; // zero
    for (char c : s){
        if ((unsigned char)c <= ' ') continue;
        int d = digit_of(c);
        if (d<0 || d>=base) throw std::runtime_error("invalid digit");
        res = Big::mul_small(res, (uint32_t)base);
        res = Big::add(res, Big((uint64_t)d));
    }
    return res;
}

// coeff asc powers; multiply by (x - r)
static std::vector<SBig> mul_linear(const std::vector<SBig>& a, const Big& r){
    std::vector<SBig> out(a.size()+1);
    for (size_t i=0;i<a.size();++i) out[i+1] = SBig::add(out[i+1], a[i]); // shift
    SBig mr = SBig::from(r, true); // -r
    for (size_t i=0;i<a.size();++i) out[i] = SBig::add(out[i], SBig::mul(a[i], mr));
    return out;
}

//main
int main(){
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    json j = json::parse(std::cin, nullptr, false); // no exceptions
    if (j.is_discarded() || !j.contains("keys") || !j["keys"].contains("k")){
        std::cerr << "invalid json\n";
        return 1;
    }
    int k = j["keys"]["k"].is_number_integer()? j["keys"]["k"].get<int>()
                                              : std::stoi(j["keys"]["k"].get<std::string>());

    // collect "1","2",... entries
    std::vector<std::tuple<int,int,std::string>> items;
    for (auto it=j.begin(); it!=j.end(); ++it){
        if (it.key()=="keys") continue;
        int idx; try { idx = std::stoi(it.key()); } catch (...) { continue; }
        if (!it->contains("base") || !it->contains("value")) continue;
        int base = it->operator[]("base").is_number_integer()? (*it)["base"].get<int>()
                   : std::stoi((*it)["base"].get<std::string>());
        std::string val = (*it)["value"].get<std::string>();
        items.emplace_back(idx, base, std::move(val));
    }
    std::sort(items.begin(), items.end(),
              [](auto& a, auto& b){ return std::get<0>(a) < std::get<0>(b); });
    if ((int)items.size()<k){ std::cerr<<"need more roots\n"; return 1; }

    // converting first k roots
    std::vector<Big> roots; roots.reserve(k);
    try {
        for (int i=0;i<k;++i){
            int base = std::get<1>(items[i]);
            const std::string& s = std::get<2>(items[i]);
            if (base<2) throw std::runtime_error("bad base");
            roots.push_back(base_to_big(s, base));
        }
    } catch (const std::exception& e){
        std::cerr << "root parse error: " << e.what() << "\n";
        return 1;
    }

    // building polynomial
    std::vector<SBig> coeff; coeff.reserve(k+1); coeff.push_back(SBig(1));
    for (const auto& r : roots) coeff = mul_linear(coeff, r);

    // print descending
    for (int i=(int)coeff.size()-1;i>=0;--i){
        if (i!=(int)coeff.size()-1) std::cout << ' ';
        std::cout << coeff[i].toString();
    }
    std::cout << "\n";
    return 0;
}
