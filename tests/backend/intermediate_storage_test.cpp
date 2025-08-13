#include <gtest/gtest.h>
#include <sqlinq/backend/intermediate_storage.hpp>

using namespace sqlinq;

TEST(IntermediateStorageTest, InitiallyEmpty) {
    IntermediateStorage<64> storage;
    EXPECT_TRUE(storage.empty());
    EXPECT_EQ(storage.size(), 0u);
    EXPECT_EQ(storage.capacity(), 64u);
}

TEST(IntermediateStorageTest, AllocateSingleInt) {
    IntermediateStorage<64> storage;
    int* ptr = static_cast<int*>(storage.allocate<int>());
    ASSERT_NE(ptr, nullptr);
    EXPECT_FALSE(storage.empty());
    EXPECT_EQ(storage.size(), sizeof(int));
}

TEST(IntermediateStorageTest, AllocateMultipleDoubles) {
    IntermediateStorage<128> storage;
    double* ptr = static_cast<double*>(storage.allocate<double>(3));
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(storage.size(), sizeof(double) * 3);
}

TEST(IntermediateStorageTest, AllocationOverflow) {
    IntermediateStorage<16> storage;
    void* ptr1 = storage.allocate<char>(10);
    ASSERT_NE(ptr1, nullptr);
    void* ptr2 = storage.allocate<char>(10);
    EXPECT_EQ(ptr2, nullptr);
}

TEST(IntermediateStorageTest, MixedTypeAllocations) {
    IntermediateStorage<64> storage;
    int* i_ptr = static_cast<int*>(storage.allocate<int>());
    double* d_ptr = static_cast<double*>(storage.allocate<double>());
    char* c_ptr = static_cast<char*>(storage.allocate<char>(5));

    ASSERT_NE(i_ptr, nullptr);
    ASSERT_NE(d_ptr, nullptr);
    ASSERT_NE(c_ptr, nullptr);

    std::size_t expectedSize = sizeof(int) + sizeof(double) + 5 * sizeof(char);
    EXPECT_EQ(storage.size(), expectedSize);
}

