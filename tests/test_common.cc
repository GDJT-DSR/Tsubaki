#include "common/common.h"
#include "plugins/file_list.h"
#include <gtest/gtest.h>
#include <sstream>

TEST(LoadFilesFromStream, ParsesTsubakiFormat) {
    std::istringstream is("aabbcc /tmp/a\n"
                          "# a comment\n"
                          "\n"
                          "  dd /tmp/b\n");
    plugins::FileList fl;
    common::loadFilesFromStream(fl, is, "test", "SUM");

    ASSERT_EQ(fl.size(), 2u);
    bool has_a = false;
    bool has_b = false;
    for (const auto &[key, val] : fl) {
        if (key == "/tmp/a") {
            has_a = true;
            EXPECT_EQ(val.hash, "aabbcc");
        } else if (key == "/tmp/b") {
            has_b = true;
            EXPECT_EQ(val.hash, "dd");
        }
    }
    EXPECT_TRUE(has_a);
    EXPECT_TRUE(has_b);
}

TEST(LoadFilesFromStream, RejectsInvalidHex) {
    std::istringstream is("zzz /tmp/a\n");
    plugins::FileList fl;
    common::loadFilesFromStream(fl, is, "test", "SUM");
    EXPECT_TRUE(fl.empty());
}

TEST(LoadFilesFromStream, RejectsLineWithoutPath) {
    std::istringstream is("aabbcc\n");
    plugins::FileList fl;
    common::loadFilesFromStream(fl, is, "test", "SUM");
    EXPECT_TRUE(fl.empty());
}
