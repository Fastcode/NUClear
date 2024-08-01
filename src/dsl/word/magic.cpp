

// template <typename T, typename U>
// std::optional<T> call(std::unique_ptr<U> msg, const NUClear::clock::duration& timeout) {

//     std::mutex m;
//     std::condition_variable cv;
//     std::unique_lock<std::mutex> lock(m);

//     std::optional<T> result;
//     powerplant.add_idle_task(unique_identifier, pool_descriptor, [&] {
//         std::unique_lock<std::mutex> lock(m);
//         cv.notify_all();
//     });

//     NUClear::dsl::store::TypeCallbackStore<T>::get().push_back([&] {
//         std::unique_lock<std::mutex> lock(m);
//         result = store::ThreadStore<std::shared_ptr<T>>::value == nullptr
//                      ? store::DataStore<DataType>::get()
//                      : *store::ThreadStore<std::shared_ptr<T>>::value;
//         cv.notify_all();
//     });

//     // TODO RAII the removal of the idle task and the trigger callback

//     cv.wait(lock, [&] { return result.has_value(); });

//     return result;
// }
