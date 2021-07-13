class ControlPrinter
{
    sendVariable = (name, value) => {
        console.log("<ctrl>:" + name + "=" + value);
    }
}

export default ControlPrinter;