void* netq_malloc_node(size_t size) {
    return malloc(1024+size);
}

void* _TIFFmalloc(size_t size) {
    return malloc(size);
}

void* png_malloc_base(void* pp, size_t s) {
    return malloc(s);
}


void* _TIFFcalloc(size_t nmemb, size_t size) {
    return calloc(nmemb, size);
}
