namespace NUClear {
    template <typename TData>
    std::shared_ptr<TData> ReactorController::get() {
        return reactormaster.get<TData>();
    }

    template <typename TReactor>
    void ReactorController::install() {
        reactormaster.install<TReactor>();
    }

    template <typename TTrigger>
    void ReactorController::emit(TTrigger* data) {
        reactormaster.emit(data);
    }
}
