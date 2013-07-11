#TODO Until NUCLear 1.0
* Make setting a url of the form epgm:// in the network name use the url rather then the hash
* Move the murmurhash3 code into its own file and add the licence for it at the top
* Build the Reactor test harness
* Setup these functions to be publically overrideable (pretty much just make sure that defining new specializations in other files is supported)
    * Emit
* Fix the Triggers so that they respect And triggers (multiple triggers) properly. To do this remove bindTriggersImpl and instead make them partially specialized types of TriggerType (such that TriggerType<Type>::type is the type we are triggering on, with std::ignore or something similar as ignoring the type) that way, bindTriggers can simply bind the callback to that type in the callback cache, and use exists for trigger logic. Then in bindTriggers we can setup the trigger specific logic for the callback to check when all the triggers have fired. (Jake if you're reading this ask me about it)