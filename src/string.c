int strcmp(const char *s1, const char *s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i])
            return s1[i] - s2[i]; // return difference
        i++;
    }
    // If one string ended, return difference of last characters
    return s1[i] - s2[i];
}
