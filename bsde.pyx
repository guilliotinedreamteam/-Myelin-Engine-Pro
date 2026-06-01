from libc.stdint cimport uint8_t, uint16_t
from libcpp.vector cimport vector

cdef extern from "bsde.hpp" namespace "myelin":
    cdef cppclass BSDE:
        BSDE(int channels)
        vector[uint8_t] encode(const uint16_t* frame, int samples)
        vector[uint16_t] decode(const uint8_t* bits, int bits_len, int samples)

cdef extern from "ass.hpp" namespace "myelin":
    cdef cppclass ASS:
        ASS(int total_channels, int cluster_size)
        void apply(uint16_t* frame, int samples)
        void reconstruct(uint16_t* frame, int samples)

cdef class PyBSDE:
    cdef BSDE* thisptr
    def __cinit__(self, int channels):
        self.thisptr = new BSDE(channels)
    def __dealloc__(self):
        del self.thisptr
    
    def encode(self, uint16_t[:] frame, int samples):
        cdef vector[uint8_t] result = self.thisptr.encode(&frame[0], samples)
        return bytearray(result)
    
    def decode(self, uint8_t[:] bits, int samples):
        cdef int bits_len = bits.shape[0]
        cdef vector[uint16_t] result = self.thisptr.decode(&bits[0], bits_len, samples)
        return list(result)

cdef class PyASS:
    cdef ASS* thisptr
    def __cinit__(self, int total_channels, int cluster_size):
        self.thisptr = new ASS(total_channels, cluster_size)
    def __dealloc__(self):
        del self.thisptr
        
    def apply(self, uint16_t[:] frame, int samples):
        self.thisptr.apply(&frame[0], samples)
        return frame
        
    def reconstruct(self, uint16_t[:] frame, int samples):
        self.thisptr.reconstruct(&frame[0], samples)
        return frame
