export class ErrorObj {
    errorMsg: any;
    causityChain: string[];

    constructor(rootCause: string, message: any) {
        this.errorMsg = message;
        this.causityChain = [rootCause];
    }

    appendCause(cause: string) {
        this.causityChain = [cause].concat(this.causityChain);
    }

    toString(): string {
        return this.causityChain.reduce((previousValue: string, currentValue: string) => previousValue+"->"+currentValue)+": "+this.errorMsg;
    }
}
