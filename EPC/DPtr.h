/*
 * NV Lib component.
 *
 * Copyright (c) 2018 Nephovision Inc.
 * 
 * author: nszjh 
 */
#ifndef  DPTR_H
#define  DPTR_H

template <typename T>
using DPtr = std::shared_ptr<T>;

template <typename T>
using Function = std::function<T>;

#endif