#include "../src/flow/flow.h"
source("src/flow/flow.c")

{
    FlowInit();

    describe("Initializing Flow module") do
        flow_dev_t *flow_dev = NULL;
        flow_dev = FlowGetDev();

        it("flow dev must be created") do
            (intptr_t)(flow_dev) not_to_eq((intptr_t)NULL);
        end
        it("state machine must be valid") do
            FlowStateGet() to_eq(FLOW_STATE_DISABLE);
        end
        it("balance must be 0") do
            flow_dev->balance to_eq(0);
        end
    end

    describe("FlowGetDev") do
        it("pointer must be taken correctly") do
            flow_dev_t *flow_dev;
            bool test_val;

            flow_dev = FlowGetDev();

            FlowGetTempVal() to_eq(false);
            flow_dev->temp_val = true;
            FlowGetTempVal() to_eq(true);
        end
    end

    context("When get balance") do
        flow_dev_t *flow_dev = NULL;
        uint32_t    balance = 42;

        FlowInit();
        flow_dev = FlowGetDev();

        it("must be taken correctly") do
            FlowGetBalance() to_eq(0);
            flow_dev->balance = balance;
            FlowGetBalance() to_eq(balance);
        end
    end

    describe("FlowCycle") do
        flow_dev_t *flow_dev = NULL;
        uint8_t test_buf_1[20] = { 0x5A, 0xA5, 0x06, 0x83, 0x10, 0x00, 0x01, 0x00, 0x01};

        FlowInit();
        flow_dev = FlowGetDev();

        context("When push button in Idle mode, balance=0") do

            DwinInit(NULL);
            for (int i = 0; i < 9; i++) DwinGetCharHandler(test_buf_1[i]);

            flow_dev->state = FLOW_STATE_IDLE;
            FlowCycle();

            it("state must be correct") do
                flow_dev->state to_eq(FLOW_STATE_CASHLESS_EN);
            end
        end
        context("When push button in Idle mode, balance=120") do
            uint32_t    balance = 120;

            DwinInit(NULL);
            for (int i = 0; i < 9; i++) DwinGetCharHandler(test_buf_1[i]);

            flow_dev->state = FLOW_STATE_IDLE;
            flow_dev->balance = 120; 
            FlowCycle();

            it("state must be correct") do
                flow_dev->state to_eq(FLOW_STATE_VEND);
            end
        end
    end

    // context("When the Vending is Disabled") do
        // it("coin box must be disabled") do
        //    FlowCoinBoxIsEnable() not_to_eq(true);
        // end

        // it("cashless must be disabled") do
        //     FlowCashlessIsEnable() not_to_eq(true);
        // end

        // it("items must be disabled") do
        //     FlowItemsIsEnabled() not_to_eq(true);
        // end

        // it("screen should indicate that the machine is not working") do
        //     FlowScreenGetPage() to_eq(FLOW_SCREEN_NOT_WORK);
        // end
    // end

    // context("When the Vending is Enabled") do
    //     describe("FlowEnable") do
    //         flow_dev_t *flow_dev;
    //         flow_dev = FlowGetDev();

    //         it("checking status before enabling ") do
    //             flow_dev.state to_eq(FLOW_STATE_DISABLE);
    //         end

    //         FlowEnable();

    //         it("state machine must be valid") do
    //             flow_dev.state to_eq(FLOW_STATE_IDLE);
    //         end
    //     end

    //     it("coin box must be enabled") do
    //         FlowCoinBoxIsEnable() to_eq(true);
    //     end

    //     it("cashless must be enabled") do
    //         FlowCashlessIsEnable() to_eq(true);
    //     end

    //     it("items must be enabled") do
    //         FlowItemsIsEnabled() to_eq(true);
    //     end

    //     it("screen should display a page with items") do
    //         FlowScreenGetPage() to_eq(FLOW_SCREEN_ITEMS);
    //     end
    // end

    // conext("When selecting a product in idle mode") do
    //     uint8_t  item_button;
    //     uint8_t  ref_button = 1;
    //     uint8_t  another_button = 2;

    //     describe("FlowEmulatePushButton") do

    //         it("checking status before pushing") do
    //             flow_dev.state to_eq(FLOW_STATE_IDLE);
    //         end

    //         FlowEmulatePushButton(ref_button);

    //         it("state machine must be valid") do
    //             flow_dev.state to_eq(FLOW_STATE_SEL_ITEM);
    //         end

    //         it("correct button must be pressed") do
    //             FlowGetPushButton() to_eq(ref_button);
    //         end

    //         it("prohibited to press another button") do
    //             item_button = FlowGetPushButton();

    //             FlowEmulatePushButton(another_button);

    //             FlowGetPushButton() to_eq(item_button);
    //         end
    //     end
    // end

    // describe("FlowBalanceSet") do
    //     uint32_t ref_balance = 50;
    //     uint32_t balance;

    //     FlowBalanceSet(ref_balance);

    //     it("balance must be right") do
    //         FlowBalanceGet() to_eq(ref_balance);
    //     end
    // end

    context("When in item selection mode and there are enough funds") do
        // uint32_t balance = 50;
        // uint32_t item_price;
        // uint8_t  button = 1;

        // FlowInit();
        // FlowEnable();
        // FlowBalanceSet(ref_balance);
        // FlowEmulatePushButton(button);
        // balance = FlowBalanceGet();

        // // it("screen should display a page with items") do
        // //     FlowScreenGetPage() to_eq(FLOW_SCREEN_ITEMS);
        // // end

        // // it("") do
        // // end

        // // item_button = Flow
        // // FlowGetItemPrice()
    end

    // conext("When in item selection mode and there are not enough funds") do
    // end

    // describe("Foo") do
    //     /*FlowInit();*/
    //     it("flow dev must be created") do
    //         assert_false;
    //     end
        
    // end
}
