//
// Created by huangyuyang on 6/13/23.
//

#include "utils.h"

#include "executor.h"

#include "devices/cpu/cpudevice.h"

#ifdef USE_CUDA
#include "devices/cuda/cudadevice.h"
#endif

namespace fastllm {
    Executor::Executor() {
        this->devices.clear();
#ifdef USE_CUDA
        this->devices.push_back((BaseDevice*) new CudaDevice());
#endif
        this->devices.push_back((BaseDevice*) new CpuDevice());
    }

    Executor::~Executor() {
        for (int i = 0; i < devices.size(); i++) {
            delete devices[i];
        }
    }

    void Executor::ClearDevices() {
        this->devices.clear();
    }

    void Executor::AddDevice(fastllm::BaseDevice *device) {
        this->devices.push_back(device);
    }

    void Executor::Run(const std::string &opType, const fastllm::DataDict &datas, const fastllm::FloatDict &floatParams,
                       const fastllm::IntDict &intParams) {
//auto st = std::chrono::system_clock::now();
        bool lockInCPU = false;
        for (auto &it : datas) {
            lockInCPU |= it.second->lockInCPU;
        }
        for (auto device : devices) {
            if (lockInCPU && device->deviceType != "cpu") {
                continue;
            }
            if (device->deviceType == "cuda" && device->CanRun(opType, datas, floatParams, intParams)) {
                auto it = datas.find("weight");
                if (device->MemoryCheck() || it->second->dataDevice == DataDevice::CUDA) {
                    for (auto &it : datas) {
                        it.second->ToDevice((void*)device);
                    }
                    device->Reshape(opType, datas, floatParams, intParams);
                    device->Run(opType, datas, floatParams, intParams);
                    break;
                }
            }
            else if (device->deviceType == "cpu") {
                for (auto &it : datas) {
                    it.second->ToDevice((void*)device);
                }
                device->Reshape(opType, datas, floatParams, intParams);
                device->Run(opType, datas, floatParams, intParams);
                break;
            }
        }
//printf("%s spend %f s.\n", opType.c_str(), GetSpan(st, std::chrono::system_clock::now()));
    }
}