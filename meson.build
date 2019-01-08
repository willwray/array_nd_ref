project('array_nd', 'cpp', default_options : 'cpp_std=c++2a')
src = ['array_nd_ref.hpp']

test('test array_nd',
  executable('array_nd', 'test/array_nd.cpp',
             cpp_args : '-fconcepts')
)

