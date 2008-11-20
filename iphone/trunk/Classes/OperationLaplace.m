//
//  OperationIntegrate.m
//  mathomatic
//
//  Created by Ben Gotow on 10/25/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "OperationLaplace.h"
#import "mathomaticViewController.h"


@implementation OperationLaplace

- (void)run
{
    UIActionSheet * sheet = [[UIActionSheet alloc] initWithTitle:@"For variable:" delegate:self cancelButtonTitle:@"Cancel" destructiveButtonTitle:nil otherButtonTitles:@"X",@"Y",@"Z",nil];
    [sheet showInView: [delegate view]];
    [sheet release];
}

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
    NSString * variable;
    
    if (buttonIndex == 0)
        variable = @"x";
    if (buttonIndex == 1)
        variable = @"y";
    if (buttonIndex == 2)
        variable = @"z";
        
    MathCommand * c = [[MathCommand alloc] init];
    [c addInput: entry];
    [c addInput: [NSString stringWithFormat: @"laplace %@", variable]];
    [delegate performCommand: c];
    [c release];
    [self release];
}

@end