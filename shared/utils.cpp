int __stdcall CopyAndSplit(const char *src,
                 char *dst,
                 const char *chunks[],
                 int lenChunks,
                 char delim) {
  if (lenChunks <= 0) return 0;

  int numChunks = 0;
  char *pTo = dst, *currentChunkStart = dst;
  const char *pFrom = src;
  const char **currentChunkArrayItem = chunks;

  for (;;) {
    char c = *(pFrom++);

    if (c == 0 || c == delim) {
      *(pTo++) = 0;
      *(currentChunkArrayItem++) = currentChunkStart;
      currentChunkStart = pTo;

      ++numChunks;
      if (numChunks == lenChunks || c == 0) {
        return numChunks;
      }

      continue;
    }

    *(pTo++) = c;
  }
}

#if defined(TEST)
TEST(T, split) {
  const char *arr[3];
  char buf[100];

  EXPECT_EQ(CopyAndSplit("abc|1|xyz", buf, arr, 3), 3);
  EXPECT_STREQ(arr[0], "abc");
  EXPECT_STREQ(arr[1], "1");
  EXPECT_STREQ(arr[2], "xyz");

  EXPECT_EQ(CopyAndSplit("123|!|abc", buf, arr, 2), 2);
  EXPECT_STREQ(arr[0], "123");
  EXPECT_STREQ(arr[1], "!");
  EXPECT_STREQ(arr[2], "xyz");

  EXPECT_EQ(CopyAndSplit("!@#|a|123", buf, arr, 1), 1);
  EXPECT_STREQ(arr[0], "!@#");
  EXPECT_STREQ(arr[1], "!");
  EXPECT_STREQ(arr[2], "xyz");

  EXPECT_EQ(CopyAndSplit("abc|1|xyz", buf, arr, 0), 0);
  EXPECT_STREQ(arr[0], "!@#");
  EXPECT_STREQ(arr[1], "!");
  EXPECT_STREQ(arr[2], "xyz");

  EXPECT_EQ(CopyAndSplit("", buf, arr, 3), 1);
  EXPECT_STREQ(arr[0], "");
  EXPECT_EQ(CopyAndSplit("ABC", buf, arr, 3), 1);
  EXPECT_STREQ(arr[0], "ABC");
  EXPECT_EQ(CopyAndSplit("|", buf, arr, 3), 2);
  EXPECT_STREQ(arr[0], "");
  EXPECT_STREQ(arr[1], "");

  EXPECT_EQ(CopyAndSplit("ABC|DEF", buf, arr, 3), 2);
  EXPECT_EQ(CopyAndSplit("||0|||", buf, arr, 3), 3);
  EXPECT_STREQ(arr[0], "");
  EXPECT_STREQ(arr[1], "");
  EXPECT_STREQ(arr[2], "0");
}
#endif