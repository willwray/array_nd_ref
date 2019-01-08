#include <cassert>

#include "array_nd_ref.hpp"
// https://wandbox.org/permlink/z2KooPw02pbj1hyC
template <typename> struct wotype;

template <class S, typename T>
requires !std::is_array_v<T>
constexpr auto& assign(S& s, T S::* mp, T&& v)
{
    return s.*mp = std::forward<T>(v);
}

template <class S, typename T>
requires std::is_array_v<T>
constexpr auto& assign(S& s, T S::* mp, T const& v)
{
    array_nd_ref{s.*mp} = v;
    return s.*mp;
}


static_assert(array_size<int> == 1);
static_assert(array_size<int[]> == 0);
static_assert(array_size<int[][2][3]> == 0);
static_assert(array_size<int[1][2][3]> == 6);

// Sample array concept
template <typename A>
concept bool SquareArray = Rank<A> == 2
                        && Extent<A>
                        == Extent<std::remove_extent_t<A>>;

// Example generic const array function taking C-array or array_nd_ref
template <typename A>
requires SquareArray<A>
constexpr auto trace(A const& a)
{
    using std::size;
    auto sum = a[0][0];
    for (size_t i=1; i < size(a); ++i)
        sum += a[i][i];
    return sum;
}

// Example generic mutable array function (in-place transpose)
template <typename A>
requires SquareArray<A>
constexpr auto& transpose(A& a)
{
    using std::size;
    for (size_t i=0; i != size(a); ++i)
        for (size_t j=i; j != size(a); ++j)
            std::swap( a[i][j], a[j][i]);
    return a;
}

// Alternative signature taking array_ref
/*
  template <typename T, size_t N>
  requires !std::is_const_v<T>
  constexpr array_nd_ref<T[N][N]>
  transpose (array_nd_ref<T[N][N]> a)
*/

template <typename T> using Id = T;

int main()
{
{
    // Test that the referred-to data at is the same address.

    // The non-const data() member function returns the array type -
    // it is not constexpr because the array_ref class stores a pointer
    // (the decay of the array) so requires a reinterpret cast.
    int i22[2][2]{{1,2},{3,4}};            // non-const array
    auto& i22r = array_nd_ref{i22}.data(); // non-const data(), bind ref
    static_assert(std::is_same_v<decltype(i22r),decltype(i22)&>);
    assert(&i22==&i22r);
    assert(i22==i22r); // truism iff above is true

    // The const data() member function returns a pointer - the array decay
    static int const c22[2][2]{{1,2},{3,4}}; // const array (static)
    constexpr auto c22r = array_nd_ref{c22}; // constexpt array_ref
    constexpr auto c22d = c22r.data();       // const data()
    static_assert( c22d == c22);
}
{
    constexpr double cmat[3][3] {{1,2,3},{4,5,6},{7,8,9}};

    double mat[3][3];
    array_nd_ref{mat} = cmat;
    //assert(mat == cmat);

    auto mat_ref = array_nd_ref{mat};
    assert(mat_ref == mat); // deep compare, waste of time as same id

    transpose(mat);
    assert(mat_ref != cmat);

    transpose(mat_ref);
    assert(mat_ref == cmat);
}
    // An array ref is handy as string literal handle
    // with better behaviour than constexpr auto& cp = "C++";
    constexpr auto cpp = array_nd_ref{"C++"};
    static_assert( cpp == "C++");  // comparison is specified
    constexpr auto cp = cpp;       // copy is specified
    static_assert( cp == "C++");
    constexpr auto ppc = array_nd_ref{"++C"};
    swap(ppc,cpp); // EH!!!
    static_assert( cpp == "C++"); // Nothing swapped but no error
    //ppc.swap(cpp); // fails for calling non-const member

    auto a = array_nd_ref{"A"};
    auto b = array_nd_ref{"B"};
    std::swap(a,b);
    assert(a=="B" && b=="A");

// constexpr tests
{
    constexpr double oxo[3][3]
    {
        1,0,0,
        0,1,0,
        0,0,1
    };
    // constexpr local array passed as ref arg to constexpr fn
    static_assert( trace(oxo) == 3.0);

    // array_nd_ref of local array is not a constant expression
    /*constexpr*/ auto oxor = array_nd_ref{oxo};
    /*static_*/assert( trace(oxor) == 3.0);
 
    // referred-to array must have static storage duration for
    // constexpr array_nd_ref
    static
    constexpr double soxo[3][3]
    {
        1,0,0,
        0,1,0,
        0,0,1
    };
    constexpr auto soxor = array_nd_ref{soxo};
    static_assert( trace(soxor) == 3.0);

    constexpr double sum { [&soxor]() {
        using std::size;
        double s=0;
        for (unsigned i=0; i < size(soxor); ++i)
            for (unsigned j=0; j < size(soxor[i]); ++j)
                s += soxor[i][j];
        return s;
        }()
    };
    static_assert(sum == 3);

    // these accesses do not work on gcc
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71504
    //constexpr double s00 = soxor(0)(0);
    //constexpr double s10 = array_nd_ref{soxor[1]}(0);
    //constexpr double s01 = soxor(0)[1];
}
// test idempotence
{
    int i[1];
    auto ir = array_nd_ref{i};
    auto irr = array_nd_ref{ir};
    static_assert(std::is_same_v<decltype(ir),decltype(irr)>);
    ir[0] = 1;
    assert(irr[0] == 1);
}
// test copy-init
{
    bool b[2][2] {{1,1},
                  {1,1}};
    // error: assigning to an array from an initializer list
    // b = {};
    array_nd_ref{b} = {};
    assert(b[0][0] == 0 && b[0][1] == 0 &&
           b[1][0] == 0 && b[1][1] == 0);

    array_nd_ref{b} = {{0,1},
                       {1,0}};
    assert(b[0][0] == 0 && b[0][1] == 1 &&
           b[1][0] == 1 && b[1][1] == 0);

    array_nd_ref{b} = {{1}
                          };
    assert(b[0][0] == 1 && b[0][1] == 0 &&
           b[1][0] == 0 && b[1][1] == 0);

    bool c[2][2] {{0,1},
                  {1,0}};
    array_nd_ref{b} = c;
    assert(b[0][0] == 0 && b[0][1] == 1 &&
           b[1][0] == 1 && b[1][1] == 0);

    array_nd_ref{b} = b; // self assign
    assert(b[0][0] == 0 && b[0][1] == 1 &&
           b[1][0] == 1 && b[1][1] == 0);
    
    const bool cc[2][2] {{1,1},
                         {1,1}};
    array_nd_ref{b} = cc;
    //array_nd_ref{cc} = b;
}
// test deduction and deref
{
    int m[2][2] = {{1,2},{3,4}};
    auto mr = array_nd_ref{m};
    static_assert(std::is_same_v<decltype(mr),
                                 array_nd_ref<int[2][2]>>);

    constexpr int const c[2][2]{{1,2},{3,4}};
    decltype(auto) cr = array_nd_ref{c};
    // FAIL
    // auto doesn't deduce const; to keep constness use decltype(auto)
    //static_assert(std::is_same_v<decltype(cr),
    //                             array_nd_ref<int const[2][2]> const>);

    // fully indexed type is reference to underlying
    static_assert(std::is_same_v<decltype(mr[0][0]),int&>);
    static_assert(std::is_same_v<decltype(mr(0)(0)),int&>);

    static_assert(std::is_same_v<decltype(cr[0][0]),int const&>);
    static_assert(std::is_same_v<decltype(cr(0)(0)),int const&>);

    // partially indexed type with [] is reference
    static_assert(std::is_same_v<decltype(mr[0]),int(&)[2]>);
    // partially indexed type with () is array_nd_ref wrapped
    static_assert(std::is_same_v<decltype(mr(0)),array_nd_ref<int[2]>>);

    static_assert(std::is_same_v<decltype(cr[0]),int const (&)[2]>);
    //static_assert(std::is_same_v<decltype(cr(0)),array_nd_ref<int const[2]> const>);

    assert( mr[0][0] == mr(0)(0));
    mr[0][0] = -1;
    assert( mr[0][0] == mr(0)(0));
    mr(0)(0) = 1;
    assert( mr[0][0] == mr(0)(0));
}
// test swap
{
    constexpr bool A[2][2] {{0,0},
                            {0,0}};
    constexpr bool B[2][2] {{1,1},
                            {1,1}};
    bool a[2][2];
    bool b[2][2];
    array_nd_ref{a} = A;
    array_nd_ref{b} = B;

    array_nd_ref{a}.swap(b);

    assert(a[0][0] == 1 && a[0][1] == 1 &&
           a[1][0] == 1 && a[1][1] == 1);

    assert(b[0][0] == 0 && b[0][1] == 0 &&
           b[1][0] == 0 && b[1][1] == 0);

    a[0][0] = 0;
    array_nd_ref{a}.swap(a); // self swap

    assert(a[0][0] == 0 && a[0][1] == 1 &&
           a[1][0] == 1 && a[1][1] == 1);
}
// test 'sliding window' array ref
//   array_nd_ref<T[N]> takes T*
{
    using std::size;
    constexpr unsigned N = 2;
    int Ns[4][N]{};
    constexpr unsigned Ns_size = size(Ns);
    constexpr unsigned winmax = Ns_size - N + 1;

    for (unsigned i = 0; i < winmax; ++i)
    {
        array_nd_ref<int[N][N]> window{&Ns[i]};
        window.fill(i);
    }
    bool check = array_nd_ref{Ns}
                        .operator==({{0,0},{1,1},{2,2},{2,2}});
    assert(check);
}
// comparison constness
{
    int       a[4]{};
    const int c[4]{};
    auto ar = array_nd_ref{a};
    auto cr = array_nd_ref{c};
    assert(ar==ar && ar==cr && cr==ar && cr==cr);
    a[0] = 1;
    assert(ar!=cr && cr!=ar);

}
// stress test equality
{
    int a[5][6][7][8][9][10]{};
    int b[5][6][7][8][9][10]{};
    a[1][2][3][4][5][6] = 1;
    b[1][2][3][4][5][6] = 1;
    bool check_eq = array_nd_ref{a} == b;
    assert( check_eq);
    b[1][2][3][4][5][6] = 0;
    bool check_neq = array_nd_ref{a} != b;
    assert( check_neq);
}
// structured binding
{
    char cstr[][4]{"C++","++C"};
    auto cstref = array_nd_ref{cstr};

    auto& [a,b] = cstr;

   // rvalue
    auto&& [c,d] = array_nd_ref{cstr};
    assert(a==c && b==d);

    // lvalue
    auto& [e,f] = cstref;
    assert(a==e && b==f);

    e[0] = 'E';
    assert(c[0] == 'E');
    d[0] = 'D';
    assert(b[0] == 'D');
    
    const char constr[][4]{"C++","++C"};
    auto& [A,B] = constr;

    // rvalue const
    auto&& [C,D] = array_nd_ref{constr};
    assert(A==C && B==D);

    // lvalue const
    auto constref = array_nd_ref{constr};
    auto const& [E,F] = constref;
    assert(A==E && B==F);
}
// tuple access
{
    static constexpr bool ft[]{false,true};
    constexpr auto ftr = array_nd_ref{ft};
    using Tr = decltype(ftr);

    static_assert(get<1>(ftr));
    static_assert(std::tuple_size<Tr>::value == 2);
    static_assert(!std::tuple_element_t<0,Tr>{});

}
// test operator= copy assign
{
    using c24 = char[2][4];

    c24 from{};
    c24 to{};
    
    // from[0] = "abc"; // invalid array assignment
    array_nd_ref{from[0]} = "abc";
    array_nd_ref{from}[1][2] = 'g';

    array_nd_ref{to} = from;

    assert(to[0][0]=='a' && to[1][2]=='g');

    to[0][3] = 'd';
    to[1][3] = 'h';
    array_nd_ref{from[0]} = to[0];

    assert(from[0][3]=='d' && from[1][3]=='\0');

    array_nd_ref{from[0]}.swap(from[1]);
}
// array member
{
    struct cpp { char str[4]{"C++"}; };
    cpp c;
    array_nd_ref{c.str} = {};
    assert(array_nd_ref{c.str} == "\0\0\0");
    array_nd_ref{c.str} = "++C";
    assert(c.str[0] == '+');
    constexpr auto mp = &cpp::str;
    array_nd_ref{c.*mp} = "C--";
    assert(c.str[0] == 'C');
    //assign_mp<mp>(c,"fin");
}
{
    static_assert( std::is_same_v< array_nd_ref<int[1]>::iterator, int*>);

    static_assert( std::is_same_v< array_nd_ref<int[1][2]>::iterator,int(*)[2] >);

    int a[2][3]{0,1,2,3,4,5};
    auto ar = array_nd_ref{a};
    auto it = ar.begin();
 
    static_assert (std::is_same_v<decltype(ar), array_nd_ref<int[2][3]> >);
    static_assert (std::is_same_v<decltype(it), int(*)[3] >);
    static_assert (std::is_same_v<decltype(++it), int(*&)[3] >);
    static_assert (std::is_same_v<decltype(it++), int(*)[3] >);
    static_assert (std::is_same_v<decltype(*it), int(&)[3] >);
    static_assert (std::is_same_v<decltype(it[0]), int(&)[3] >);
}
}