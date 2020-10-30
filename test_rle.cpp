#include "rle.h"
#include "rle_io.h"

#include <iostream>
#include <gtest/gtest.h> // googletest header file


std::vector<int> ref1 = { 0,1,2,3,4 };
std::vector<int> ref2 = { 0,0 };
std::vector<int> ref3 = { 0,0,1,1,1,2,2,2,2,3,3,3,3,3,4,4,4,4,4,4 };
std::vector<int> ref4 = { };
std::vector<int> ref5 = { 9 };
std::vector<int> ref6 = { 0,0,1,1,2,2,3,3,4,4,5,5 };


std::vector<int> test_io(std::vector<int> v) {
    rle<int> rle(v);
    rle_io<int> out_rle(rle);
    out_rle.to_file("tmp.rle");

    rle_io<int> in_rle;
    in_rle.from_file("tmp.rle");
    return in_rle.get().decode();
}

std::vector<int> enc1(std::vector<int> v) {
    return rle<int>(v).decode();
}

std::vector<int> enc2(std::vector<int> arr) {
    rle<int> a;
    for(auto val : arr) {
        a << val;
    }
    return a.decode();
}

std::vector<int> enc3(std::vector<int> arr) {
    rle<int> a(arr);
    
    std::vector<int> b;
    for(int i = 0; i < arr.size(); i++) {
        int v = *a[i];
        b.push_back(v);
    }
    return b;
}

std::vector<int> enc4(std::vector<int> arr) {
    rle<int> a(arr);
    a.set(0, 9);
    auto n =  a.decode();
    for(auto v : n)
        std::cerr << v << std::endl;

    std::cerr << "new 1" << std::endl;
    a = rle<int>(arr);
    a.set(1, 9);
    n =  a.decode();
    for(auto v : n)
        std::cerr << v << std::endl;

    std::cerr << "new 0" << std::endl;
    a = rle<int>(arr);
    a.set(2, 9);
    n =  a.decode();
    for(auto v : n)
        std::cerr << v << std::endl;

    std::cerr << "new 3" << std::endl;
    a = rle<int>(arr);
    a.set(3, 9);
    n =  a.decode();
    for(auto v : n)
        std::cerr << v << std::endl;

    std::cerr << "new 4" << std::endl;
    a = rle<int>(arr);
    a.set(4, 9);
    n =  a.decode();
    for(auto v : n)
        std::cerr << v << std::endl;

    std::cerr << "new 5" << std::endl;
    a = rle<int>(arr);
    a.set(5, 9);
    n =  a.decode();
    for(auto v : n)
        std::cerr << v << std::endl;

    std::cerr << "new 6" << std::endl;
    a = rle<int>(arr);
    a.set(6, 9);
    n =  a.decode();
    for(auto v : n)
        std::cerr << v << std::endl;

    std::cerr << "new 11" << std::endl;
    a = rle<int>(arr);
    a.set(11, 9);
    n =  a.decode();
    for(auto v : n)
        std::cerr << v << std::endl;

    std::cerr << "new 12" << std::endl;
    a = rle<int>(arr);
    a.set(12, 9);
    n =  a.decode();
    for(auto v : n)
        std::cerr << v << std::endl;
    return n;
}

auto test1 = enc1(ref1);
auto test2 = enc1(ref2);
auto test3 = enc1(ref3);
auto test4 = enc1(ref4);
auto test5 = enc1(ref5);
auto test6 = enc2(ref3);
auto test7 = enc3(ref3);
auto test8 = test_io(ref6);


TEST(RLETest, ArrayComparison1) {
    ASSERT_EQ(ref1.size(), test1.size()) << "Vectors x and y are of unequal length";

    for (int i = 0; i < ref1.size(); ++i) {
        EXPECT_EQ(ref1[i], test1[i]) << "Vectors x and y differ at index " << i;
    }
}

TEST(RLETest, ArrayComparison2) {
    ASSERT_EQ(ref2.size(), test2.size()) << "Vectors x and y are of unequal length";

    for (int i = 0; i < ref2.size(); ++i) {
        EXPECT_EQ(ref2[i], test2[i]) << "Vectors x and y differ at index " << i;
    }
}

TEST(RLETest, ArrayComparison3) {
    ASSERT_EQ(ref3.size(), test3.size()) << "Vectors x and y are of unequal length";

    for (int i = 0; i < ref3.size(); ++i) {
        EXPECT_EQ(ref3[i], test3[i]) << "Vectors x and y differ at index " << i;
    }
}

TEST(RLETest, ArrayComparison4) {
    ASSERT_EQ(ref4.size(), test4.size()) << "Vectors x and y are of unequal length";

    for (int i = 0; i < ref4.size(); ++i) {
        EXPECT_EQ(ref4[i], test4[i]) << "Vectors x and y differ at index " << i;
    }
}

TEST(RLETest, ArrayComparison5) {
    ASSERT_EQ(ref5.size(), test5.size()) << "Vectors x and y are of unequal length";

    for (int i = 0; i < ref5.size(); ++i) {
        EXPECT_EQ(ref5[i], test5[i]) << "Vectors x and y differ at index " << i;
    }
}


TEST(RLETest, ArrayComparison6) {
    ASSERT_EQ(ref3.size(), test6.size()) << "Vectors x and y are of unequal length";

    for (int i = 0; i < ref3.size(); ++i) {
        EXPECT_EQ(ref3[i], test6[i]) << "Vectors x and y differ at index " << i;
    }
}

TEST(RLETest, ArrayComparison7) {
    ASSERT_EQ(ref3.size(), test7.size()) << "Vectors x and y are of unequal length";

    for (int i = 0; i < ref3.size(); ++i) {
        EXPECT_EQ(ref3[i], test7[i]) << "Vectors x and y differ at index " << i;
    }
}


TEST(RLETest, ArrayComparison8) {
    ASSERT_EQ(ref6.size(), test8.size()) << "Vectors x and y are of unequal length";

    for (int i = 0; i < ref6.size(); ++i) {
        EXPECT_EQ(ref6[i], test8[i]) << "Vectors x and y differ at index " << i;
    }
}
