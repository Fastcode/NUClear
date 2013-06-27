#TODO Until NUCLear 1.0
* Make setting a url of the form epgm:// in the network name use the url rather then the hash
* Move the murmurhash3 code into its own file and add the licence for it at the top
* Change emit(type) and networkEmit(type) to be implemented as emit(type) emit<Network>(type) and emit<Network, Local>(type). This will require converting emit to a functor as this is a partial specization (the rational behind this is that while emitting both network and local we can pass the object to the network first then local (this has to do with the ownership of the data potentially being lost once it goes to local))
* Build the Reactor test harness
* Setup these functions to be publically overrideable (pretty much just make sure that defining new specializations in other files is supported)
    * Emit
    * Get
    * Fill
    * BindTriggerImpl (also possibly rename this)
    * Exists