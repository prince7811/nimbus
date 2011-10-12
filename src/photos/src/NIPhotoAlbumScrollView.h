//
// Copyright 2011 Jeff Verkoeyen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "NIPagingScrollView.h"
#import "NIPagingScrollViewDataSource.h"
#import "NIPagingScrollViewDelegate.h"
#import "NIPhotoScrollView.h"

@protocol NIPhotoAlbumScrollViewDataSource;
@protocol NIPhotoAlbumScrollViewDelegate;

/**
 * A paged scroll view that shows a collection of photos.
 *
 *      @ingroup Photos-Views
 *
 * This view provides a light-weight implementation of a photo viewer, complete with
 * pinch-to-zoom and swiping to change photos. It is designed to perform well with
 * large sets of photos and large images that are loaded from either the network or
 * disk.
 *
 * It is intended for this view to be used in conjunction with a view controller that
 * implements the data source protocol and presents any required chrome.
 *
 *      @see NIToolbarPhotoViewController
 */
@interface NIPhotoAlbumScrollView : NIPagingScrollView <NIPhotoScrollViewDelegate> {
@private
  // Configurable Properties
  UIImage* _loadingImage;
  BOOL _zoomingIsEnabled;
  BOOL _zoomingAboveOriginalSizeIsEnabled;
}

#pragma mark Data Source /** @name Data Source */

/**
 * The data source for this photo album view.
 *
 * This is the only means by which this photo album view acquires any information about the
 * album to be displayed.
 */
@property (nonatomic, readwrite, assign) id<NIPhotoAlbumScrollViewDataSource> dataSource;

/**
 * Stock implementation of NIPagingScrollViewDatasource's method for creating a page.
 */
- (id<NIPagingScrollViewPage>)pagingScrollView:(NIPagingScrollView *)pagingScrollView pageForIndex:(NSInteger)pageIndex;


#pragma mark Delegate /** @name Delegate */

/**
 * The delegate for this photo album view.
 *
 * Any user interactions or state changes are sent to the delegate through this property.
 */
@property (nonatomic, readwrite, assign) id<NIPhotoAlbumScrollViewDelegate> delegate;


#pragma mark Configuring Functionality /** @name Configuring Functionality */

/**
 * Whether zooming is enabled or not.
 *
 * Regardless of whether this is enabled, only original-sized images will be zoomable.
 * This is because we often don't know how large the final image is so we can't
 * calculate min and max zoom amounts correctly.
 *
 * By default this is YES.
 */
@property (nonatomic, readwrite, assign, getter=isZoomingEnabled) BOOL zoomingIsEnabled;

/**
 * Whether small photos can be zoomed at least until they fit the screen.
 *
 *      @see NIPhotoScrollView::zoomingAboveOriginalSizeIsEnabled
 *
 * By default this is YES.
 */
@property (nonatomic, readwrite, assign, getter=isZoomingAboveOriginalSizeEnabled) BOOL zoomingAboveOriginalSizeIsEnabled;


#pragma mark Configuring Presentation /** @name Configuring Presentation */

/**
 * An image that is displayed while the photo is loading.
 *
 * This photo will be presented if no image is returned in the data source's implementation
 * of photoAlbumScrollView:photoAtIndex:photoSize:isLoading:.
 *
 * Zooming is disabled when showing a loading image, regardless of the state of zoomingIsEnabled.
 *
 * By default this is nil.
 */
@property (nonatomic, readwrite, retain) UIImage* loadingImage;


#pragma mark Notifying the View of Loaded Photos /** @name Notifying the View of Loaded Photos */

/**
 * Notify the scroll view that a photo has been loaded at a given index.
 *
 * You should notify the completed loading of thumbnails as well. Calling this method
 * is fairly lightweight and will only update the images of the visible pages. Err on the
 * side of calling this method too much rather than too little.
 *
 * The photo at the given index will only be replaced with the given image if photoSize
 * is of a higher quality than the currently-displayed photo's size.
 */
- (void)didLoadPhoto: (UIImage *)image
             atIndex: (NSInteger)photoIndex
           photoSize: (NIPhotoScrollViewPhotoSize)photoSize;

@end


/**
 * The photo album scroll data source.
 *
 *      @ingroup Photos-Protocols
 *
 * This data source emphasizes speed and memory efficiency by requesting images only when
 * they're needed and encouraging immediate responses from the data source implementation.
 *
 *      @see NIPhotoAlbumScrollView
 */
@protocol NIPhotoAlbumScrollViewDataSource <NIPagingScrollViewDataSource>

@required

#pragma mark Fetching Required Album Information /** @name [NIPhotoAlbumScrollViewDataSource] Fetching Required Album Information */

/**
 * Fetches the highest-quality image available for the photo at the given index.
 *
 * Your goal should be to make this implementation return as fast as possible. Avoid
 * hitting the disk or blocking on a network request. Aim to load images asynchronously.
 *
 * If you already have the highest-quality image in memory (like in an NIImageMemoryCache),
 * then you can simply return the image and set photoSize to be
 * NIPhotoScrollViewPhotoSizeOriginal.
 *
 * If the highest-quality image is not available when this method is called then you should
 * spin off an asynchronous operation to load the image and set isLoading to YES.
 *
 * If you have a thumbnail in memory but not the full-size image yet, then you should return
 * the thumbnail, set isLoading to YES, and set photoSize to NIPhotoScrollViewPhotoSizeThumbnail.
 *
 * Once the high-quality image finishes loading, call didLoadPhoto:atIndex:photoSize: with
 * the image.
 *
 * This method will be called to prefetch the next and previous photos in the scroll view.
 * The currently displayed photo will always be requested first.
 *
 *      @attention The photo scroll view does not hold onto the UIImages for very long at all.
 *                 It is up to the controller to decide on an adequate caching policy to ensure
 *                 that images are kept in memory through the life of the photo album.
 *                 In your implementation of the data source you should prioritize thumbnails
 *                 being kept in memory over full-size images. When a memory warning is received,
 *                 the original photos should be relinquished from memory first.
 */
- (UIImage *)photoAlbumScrollView: (NIPhotoAlbumScrollView *)photoAlbumScrollView
                     photoAtIndex: (NSInteger)photoIndex
                        photoSize: (NIPhotoScrollViewPhotoSize *)photoSize
                        isLoading: (BOOL *)isLoading
          originalPhotoDimensions: (CGSize *)originalPhotoDimensions;

@optional

#pragma mark Optimizing Data Retrieval /** @name [NIPhotoAlbumScrollViewDataSource] Optimizing Data Retrieval */

/**
 * Called when you should cancel any asynchronous loading requests for the given photo.
 *
 * When a photo is not immediately visible this method is called to allow the data
 * source to minimize the number of active asynchronous operations in place.
 *
 * This method is optional, though recommended because it focuses the device's processing
 * power on the most immediately accessible photos.
 */
- (void)photoAlbumScrollView: (NIPhotoAlbumScrollView *)photoAlbumScrollView
     stopLoadingPhotoAtIndex: (NSInteger)photoIndex;

@end


/**
 * The photo album scroll view delegate.
 *
 *      @ingroup Photos-Protocols
 *      @see NIPhotoAlbumScrollView
 */
@protocol NIPhotoAlbumScrollViewDelegate <NIPagingScrollViewDelegate>

@optional

#pragma mark Scrolling and Zooming /** @name [NIPhotoAlbumScrollViewDelegate] Scrolling and Zooming */

/**
 * The user double-tapped to zoom in or out.
 */
- (void)photoAlbumScrollView: (NIPhotoAlbumScrollView *)photoAlbumScrollView
                   didZoomIn: (BOOL)didZoomIn;


#pragma mark Data Availability /** @name [NIPhotoAlbumScrollViewDelegate] Data Availability */

/**
 * The next photo in the album has been loaded and is ready to be displayed.
 */
- (void)photoAlbumScrollViewDidLoadNextPhoto:(NIPhotoAlbumScrollView *)photoAlbumScrollView;

/**
 * The previous photo in the album has been loaded and is ready to be displayed.
 */
- (void)photoAlbumScrollViewDidLoadPreviousPhoto:(NIPhotoAlbumScrollView *)photoAlbumScrollView;


@end
